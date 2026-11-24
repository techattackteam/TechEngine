#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/GpuCopies.hpp>
#include <render/MaterialBuffer.hpp>
#include <render/device/RenderDevice.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

using TechEngine::FinishedUpload;
using TechEngine::FramePacket;
using TechEngine::GeometryRange;
using TechEngine::GpuCopies;
using TechEngine::GpuCopy;
using TechEngine::GpuMaterial;
using TechEngine::GpuMeshSlot;
using TechEngine::Material;
using TechEngine::Mesh;
using TechEngine::MeshSlot;
using TechEngine::PacketInstance;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceUploadKind;
using TechEngine::ResourceUploadRecord;
using TechEngine::Texture;
using TechEngine::TextureFormat;
using TechEngine::Vec3;
using TechEngine::Vec4;

static constexpr ResourceId MESH_ID{0x5001};
static constexpr ResourceId MATERIAL_ID{0x5002};
static constexpr ResourceId TEXTURE_ID{0x5003};
static constexpr ResourceId UNRELATED_ID{0x5999};

static constexpr ResourceSlotId MESH_SLOT{1, 0};
static constexpr ResourceSlotId MATERIAL_SLOT{2, 0};
static constexpr ResourceSlotId TEXTURE_SLOT{3, 0};
static constexpr ResourceSlotId SECOND_MESH_SLOT{4, 0};
static constexpr ResourceSlotId SECOND_MATERIAL_SLOT{5, 0};
static constexpr ResourceSlotId SECOND_TEXTURE_SLOT{6, 0};

static constexpr ResourceId SECOND_TEXTURE_ID{0x5004};
static constexpr ResourceId SECOND_MATERIAL_ID{0x5005};

static constexpr std::size_t BASE_COLOR_INPUT = 0;
static constexpr std::size_t PACKED_ORM_INPUT = 1;
static constexpr std::size_t NORMAL_INPUT = 2;
static constexpr std::size_t EMISSIVE_INPUT = 3;

static constexpr std::uint64_t FIRST_FRAME = 1;

static constexpr std::array<const FramePacket*, 2> NO_PACKETS{};

static ResourceUploadRecord uploadOf(const ResourceSlotId slot, const TechEngine::StringId typeTag, const ResourceId id, const void* payload, const std::uint64_t token, std::vector<ResourceSlotId> dependencies = {}) {
    ResourceUploadRecord record;
    record.kind = ResourceUploadKind::Upload;
    record.slot = slot;
    record.typeTag = typeTag;
    record.id = id;
    record.payload = payload;
    record.dependencies = std::move(dependencies);
    record.token = token;
    return record;
}

template<typename Body>
static bool runOnContext(TechEngine::Window& window, Body&& body) {
    bool loaded = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0 && GLAD_GL_ARB_bindless_texture != 0;
            if (loaded) {
                body();
            }
            window.releaseContext();
        });
    }
    return loaded;
}

static Texture makeTexture(const TextureFormat format, const std::uint32_t width, const std::uint32_t height) {
    Texture texture;
    texture.format = format;
    texture.width = width;
    texture.height = height;
    const std::uint32_t levels = Texture::mipCount(width, height);
    for (std::uint32_t level = 0; level < levels; level++) {
        texture.mips.emplace_back(static_cast<std::size_t>(Texture::mipByteSize(format, width, height, level)));
    }
    return texture;
}

static std::uint64_t handleAt(const GpuMaterial& record, const std::size_t input) {
    return static_cast<std::uint64_t>(record.textureHandles[input * 2]) | (static_cast<std::uint64_t>(record.textureHandles[input * 2 + 1]) << 32U);
}

static ResourceUploadRecord releaseOf(const ResourceSlotId slot, const TechEngine::StringId typeTag, const ResourceId id) {
    ResourceUploadRecord record;
    record.kind = ResourceUploadKind::Release;
    record.slot = slot;
    record.typeTag = typeTag;
    record.id = id;
    return record;
}

TEST_CASE("an upload stores a copy under its slot with its id label and dependency slots, and reports its slot and token", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh mesh{};
    const std::vector<ResourceUploadRecord> records{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 7, {MATERIAL_SLOT})};
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 7}});
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->typeTag == TechEngine::MESH_TYPE_TAG);
    REQUIRE(copy->id == MESH_ID);
    REQUIRE(copy->dependencies == std::vector<ResourceSlotId>{MATERIAL_SLOT});
    REQUIRE_FALSE(copy->released);
}

TEST_CASE("a dependency is found through the upload's slot and the label through its id, never through the payload's ids", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "GpuCopies dependency test") == std::error_code{});

    std::error_code initializeError;
    bool materialFound = false;
    ResourceId materialId;
    std::vector<ResourceSlotId> materialDependencies;
    bool textureFound = false;
    ResourceId textureId;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        GpuCopies copies;
        initializeError = copies.initialize(device);
        const Texture texture = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        Material material;
        material.id = UNRELATED_ID;
        material.baseColorTexture.id = UNRELATED_ID;
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 1, {TEXTURE_SLOT}),
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &texture, 2),
        };
        std::vector<FinishedUpload> finished;

        copies.apply(records, FIRST_FRAME, finished);

        const GpuCopy* materialCopy = copies.findCopy(MATERIAL_SLOT);
        materialFound = materialCopy != nullptr;
        if (materialFound) {
            materialId = materialCopy->id;
            materialDependencies = materialCopy->dependencies;
        }
        const GpuCopy* textureCopy = materialFound && !materialDependencies.empty() ? copies.findCopy(materialDependencies[0]) : nullptr;
        textureFound = textureCopy != nullptr;
        if (textureFound) {
            textureId = textureCopy->id;
        }
        copies.shutdown();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(initializeError == std::error_code{});
    REQUIRE(materialFound);
    REQUIRE(materialId == MATERIAL_ID);
    REQUIRE(materialDependencies == std::vector<ResourceSlotId>{TEXTURE_SLOT});
    REQUIRE(textureFound);
    REQUIRE(textureId == TEXTURE_ID);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("tokens are reported in record order, one per upload and none for a release", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh mesh{};
    const Mesh secondMesh{};
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 4),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
        uploadOf(SECOND_MESH_SLOT, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &secondMesh, 9),
    };
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 4}, FinishedUpload{SECOND_MESH_SLOT, 9}});
}

TEST_CASE("a second upload to the same slot replaces the copy and reports its own slot and token", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh first{};
    const Mesh second{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> load{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &first, 1, {MATERIAL_SLOT})};
    copies.apply(load, FIRST_FRAME, finished);

    const std::vector<ResourceUploadRecord> reload{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &second, 2, {TEXTURE_SLOT})};
    copies.apply(reload, FIRST_FRAME, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 1}, FinishedUpload{MESH_SLOT, 2}});
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->dependencies == std::vector<ResourceSlotId>{TEXTURE_SLOT});
}

TEST_CASE("a release marks the copy released and keeps it", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh mesh{};
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 1),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
    };
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->released);
    REQUIRE(copy->id == MESH_ID);
}

TEST_CASE("a reused slot index with a new generation is a separate copy from the released one", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh released{};
    const Mesh reused{};
    const ResourceSlotId reusedSlot{MESH_SLOT.index, MESH_SLOT.generation + 1};
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &released, 1),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
        uploadOf(reusedSlot, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &reused, 2),
    };
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    const GpuCopy* oldCopy = copies.findCopy(MESH_SLOT);
    const GpuCopy* newCopy = copies.findCopy(reusedSlot);
    REQUIRE(oldCopy != nullptr);
    REQUIRE(newCopy != nullptr);
    REQUIRE(oldCopy->released);
    REQUIRE(oldCopy->id == MESH_ID);
    REQUIRE_FALSE(newCopy->released);
    REQUIRE(newCopy->id == UNRELATED_ID);
}

TEST_CASE("a mesh with no vertices uploads before initialize and its copy holds no geometry", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh mesh{};
    const std::vector<ResourceUploadRecord> records{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 3)};
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 3}});
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->geometry == GeometryRange{});
}

TEST_CASE("a mesh's copy keeps each slot's index range and finds its default material through the upload's slots", "[client][render][upload]") {
    GpuCopies copies;
    Mesh mesh;
    mesh.slots = {
        MeshSlot{"first", 0, 6, MATERIAL_ID},
        MeshSlot{"second", 6, 3, ResourceId{}},
        MeshSlot{"third", 9, 12, UNRELATED_ID},
    };
    const std::vector<ResourceUploadRecord> records{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 1, {MATERIAL_SLOT, SECOND_MATERIAL_SLOT})};
    std::vector<FinishedUpload> finished;

    copies.apply(records, FIRST_FRAME, finished);

    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(
        copy->meshSlots == std::vector<GpuMeshSlot>{
                               GpuMeshSlot{0, 6, MATERIAL_SLOT},
                               GpuMeshSlot{6, 3, ResourceSlotId{}},
                               GpuMeshSlot{9, 12, SECOND_MATERIAL_SLOT},
                           });
}

TEST_CASE("a slot that never had an upload has no copy", "[client][render][upload]") {
    GpuCopies copies;

    REQUIRE(copies.findCopy(MESH_SLOT) == nullptr);
    REQUIRE(copies.findCopy(ResourceSlotId{}) == nullptr);
}

static PacketInstance instanceOf(const ResourceSlotId mesh) {
    PacketInstance instance;
    instance.mesh = ResourceHandle<Mesh>{mesh.index, mesh.generation};
    return instance;
}

static FramePacket packetDrawing(const ResourceSlotId mesh) {
    FramePacket packet;
    packet.instances.push_back(instanceOf(mesh));
    return packet;
}

TEST_CASE("a released copy that neither packet names is retired, and is dropped only once its stamped frame finishes", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh mesh{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> load{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 1)};
    copies.apply(load, FIRST_FRAME, finished);
    copies.retireUnreachable(NO_PACKETS, FIRST_FRAME);
    const std::vector<ResourceUploadRecord> release{releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID)};

    copies.apply(release, 3, finished);
    copies.retireUnreachable(NO_PACKETS, 3);

    REQUIRE(copies.findCopy(MESH_SLOT) == nullptr);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(0);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(1);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(2);
    REQUIRE(copies.getRetiredCount() == 0);
}

TEST_CASE("a released copy stays while either packet in the history draws it, and retires the first frame neither does", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh mesh{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 1),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
    };
    copies.apply(records, FIRST_FRAME, finished);
    const FramePacket drawing = packetDrawing(MESH_SLOT);
    const FramePacket empty;
    std::array<const FramePacket*, 2> packets{};
    SECTION("drawn by the current packet") {
        packets = {nullptr, &drawing};
    }
    SECTION("named by the previous packet only") {
        packets = {&drawing, &empty};
    }

    copies.retireUnreachable(packets, 2);

    const GpuCopy* kept = copies.findCopy(MESH_SLOT);
    REQUIRE(kept != nullptr);
    REQUIRE(kept->released);
    REQUIRE(copies.getRetiredCount() == 0);

    copies.retireUnreachable(std::array<const FramePacket*, 2>{&empty, &empty}, 3);

    REQUIRE(copies.findCopy(MESH_SLOT) == nullptr);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(1);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(2);
    REQUIRE(copies.getRetiredCount() == 0);
}

TEST_CASE("a copy that is not released is never retired, even when no packet names it", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh mesh{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> load{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 1)};
    copies.apply(load, FIRST_FRAME, finished);

    for (std::uint64_t frame = FIRST_FRAME; frame <= 4; frame++) {
        copies.retireUnreachable(NO_PACKETS, frame);
        copies.dropFinished(frame);
    }

    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE_FALSE(copy->released);
    REQUIRE(copies.getRetiredCount() == 0);
}

TEST_CASE("a packet that draws a reused slot's new generation does not keep the released old one", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh released{};
    const Mesh reused{};
    const ResourceSlotId reusedSlot{MESH_SLOT.index, MESH_SLOT.generation + 1};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &released, 1),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
        uploadOf(reusedSlot, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &reused, 2),
    };
    copies.apply(records, FIRST_FRAME, finished);
    const FramePacket drawing = packetDrawing(reusedSlot);

    copies.retireUnreachable(std::array<const FramePacket*, 2>{nullptr, &drawing}, 2);

    REQUIRE(copies.findCopy(MESH_SLOT) == nullptr);
    REQUIRE(copies.findCopy(reusedSlot) != nullptr);
    REQUIRE(copies.getRetiredCount() == 1);
}

TEST_CASE("a reload retires the old copy at once, stamped with the frame before the one being drawn", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh first{};
    const Mesh second{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> load{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &first, 1, {MATERIAL_SLOT})};
    copies.apply(load, FIRST_FRAME, finished);
    const std::vector<ResourceUploadRecord> reload{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &second, 2, {TEXTURE_SLOT})};

    copies.apply(reload, 4, finished);

    REQUIRE(copies.getRetiredCount() == 1);
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE_FALSE(copy->released);
    REQUIRE(copy->dependencies == std::vector<ResourceSlotId>{TEXTURE_SLOT});
    copies.dropFinished(2);
    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(3);
    REQUIRE(copies.getRetiredCount() == 0);
    REQUIRE(copies.findCopy(MESH_SLOT) != nullptr);
}

TEST_CASE("a copy reloaded before the first frame is drawn can be dropped at once", "[client][render][upload][drop]") {
    GpuCopies copies;
    const Mesh first{};
    const Mesh second{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &first, 1),
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &second, 2),
    };

    copies.apply(records, FIRST_FRAME, finished);

    REQUIRE(copies.getRetiredCount() == 1);
    copies.dropFinished(0);
    REQUIRE(copies.getRetiredCount() == 0);
}

struct GpuCopiesRun {
    bool loaded = false;
    std::error_code initializeError;
    unsigned int glError = GL_NO_ERROR;
};

// The body runs on the context's thread, so it records what it sees and the case asserts afterwards.
template<typename Body>
static GpuCopiesRun runWithCopies(const char* title, Body&& body) {
    GpuCopiesRun run;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, title) == std::error_code{});
    run.loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        GpuCopies copies;
        run.initializeError = copies.initialize(device);
        if (!run.initializeError) {
            body(device, copies);
        }
        copies.shutdown();
        device.shutdown();
        run.glError = glGetError();
    });
    return run;
}

static Material makeMaterial() {
    Material material;
    material.shadingModel = TechEngine::MaterialShadingModel::Unlit;
    material.alphaCutoff = 0.25F;
    material.baseColorFactor = Vec4{0.5F, 0.25F, 1.0F, 0.75F};
    material.occlusionStrength = 0.875F;
    material.roughnessFactor = 0.375F;
    material.metallicFactor = 0.625F;
    material.normalScale = 1.5F;
    material.emissiveFactor = Vec3{2.0F, 3.0F, 4.0F};
    return material;
}

TEST_CASE("a texture uploads in its file's format with every mip, and is made resident once", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    static constexpr std::array<TextureFormat, 4> FORMATS{TextureFormat::Bc7Srgb, TextureFormat::Bc7Unorm, TextureFormat::Bc5Unorm, TextureFormat::Rgb9E5};
    std::array<bool, FORMATS.size()> created{};
    std::array<bool, FORMATS.size()> resident{};
    const GpuCopiesRun run = runWithCopies("GpuCopies texture formats test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        std::vector<Texture> textures;
        for (const TextureFormat format: FORMATS) {
            textures.push_back(makeTexture(format, 8, 4));
        }
        std::vector<ResourceUploadRecord> records;
        for (std::size_t i = 0; i < textures.size(); i++) {
            records.push_back(uploadOf(ResourceSlotId{static_cast<std::uint32_t>(10 + i), 0}, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &textures[i], i + 1));
        }
        std::vector<FinishedUpload> finished;

        copies.apply(records, FIRST_FRAME, finished);

        for (std::size_t i = 0; i < records.size(); i++) {
            const GpuCopy* copy = copies.findCopy(records[i].slot);
            created[i] = copy != nullptr && copy->texture.valid();
            const std::uint64_t handle = created[i] ? device.getBindlessHandle(copy->texture) : 0;
            resident[i] = handle != 0 && glIsTextureHandleResidentARB(handle) == GL_TRUE;
        }
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    for (std::size_t i = 0; i < FORMATS.size(); i++) {
        CHECK(created[i]);
        CHECK(resident[i]);
    }
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("each material writes its 96-byte record with the handles of the inputs it names, and an empty input stays zero", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    Material material = makeMaterial();
    material.occlusionRoughnessMetallicTexture.id = TEXTURE_ID;
    material.emissiveTexture.id = SECOND_TEXTURE_ID;
    bool found = false;
    std::uint32_t recordIndex = TechEngine::DEFAULT_MATERIAL_RECORD;
    GpuMaterial record;
    std::uint64_t packedHandle = 0;
    std::uint64_t emissiveHandle = 0;
    const GpuCopiesRun run = runWithCopies("GpuCopies material record test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        const Texture packed = makeTexture(TextureFormat::Bc7Unorm, 4, 4);
        const Texture emissive = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> textures{
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &packed, 1),
            uploadOf(SECOND_TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, SECOND_TEXTURE_ID, &emissive, 2),
        };
        copies.apply(textures, FIRST_FRAME, finished);
        const std::vector<ResourceUploadRecord> materials{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 3, {TEXTURE_SLOT, SECOND_TEXTURE_SLOT})};

        copies.apply(materials, FIRST_FRAME, finished);

        const GpuCopy* materialCopy = copies.findCopy(MATERIAL_SLOT);
        found = materialCopy != nullptr;
        if (found) {
            recordIndex = materialCopy->materialRecord;
            record = copies.getMaterials().getRecord(recordIndex);
        }
        packedHandle = device.getBindlessHandle(copies.findCopy(TEXTURE_SLOT)->texture);
        emissiveHandle = device.getBindlessHandle(copies.findCopy(SECOND_TEXTURE_SLOT)->texture);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    REQUIRE(found);
    CHECK(recordIndex != TechEngine::DEFAULT_MATERIAL_RECORD);
    CHECK(record.albedo == material.baseColorFactor);
    CHECK(record.emissiveFactor == material.emissiveFactor);
    CHECK(record.alphaCutoff == material.alphaCutoff);
    CHECK(record.occlusionStrength == material.occlusionStrength);
    CHECK(record.roughnessFactor == material.roughnessFactor);
    CHECK(record.metallicFactor == material.metallicFactor);
    CHECK(record.normalScale == material.normalScale);
    CHECK(record.shadingModel == static_cast<std::uint32_t>(TechEngine::MaterialShadingModel::Unlit));
    CHECK(packedHandle != 0);
    CHECK(emissiveHandle != 0);
    CHECK(handleAt(record, BASE_COLOR_INPUT) == 0);
    CHECK(handleAt(record, PACKED_ORM_INPUT) == packedHandle);
    CHECK(handleAt(record, NORMAL_INPUT) == 0);
    CHECK(handleAt(record, EMISSIVE_INPUT) == emissiveHandle);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("a material uploaded before its texture gets the texture's handle when the texture arrives", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    Material material = makeMaterial();
    material.baseColorTexture.id = TEXTURE_ID;
    GpuMaterial record;
    std::uint64_t textureHandle = 0;
    const GpuCopiesRun run = runWithCopies("GpuCopies late texture test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        const Texture texture = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 1, {TEXTURE_SLOT}),
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &texture, 2),
        };
        std::vector<FinishedUpload> finished;

        copies.apply(records, FIRST_FRAME, finished);

        record = copies.getMaterials().getRecord(copies.findCopy(MATERIAL_SLOT)->materialRecord);
        textureHandle = device.getBindlessHandle(copies.findCopy(TEXTURE_SLOT)->texture);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(textureHandle != 0);
    CHECK(handleAt(record, BASE_COLOR_INPUT) == textureHandle);
    CHECK(handleAt(record, PACKED_ORM_INPUT) == 0);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("a texture reload rewrites every material that names it with the new handle, and leaves the others alone", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    Material reloadedUser = makeMaterial();
    reloadedUser.baseColorTexture.id = TEXTURE_ID;
    reloadedUser.emissiveTexture.id = TEXTURE_ID;
    Material otherUser = makeMaterial();
    otherUser.baseColorTexture.id = SECOND_TEXTURE_ID;
    std::uint64_t oldHandle = 0;
    std::uint64_t newHandle = 0;
    std::uint64_t otherHandle = 0;
    GpuMaterial reloadedRecord;
    GpuMaterial otherRecord;
    const GpuCopiesRun run = runWithCopies("GpuCopies texture reload test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        const Texture first = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        const Texture other = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        const Texture reloaded = makeTexture(TextureFormat::Bc7Srgb, 8, 8);
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &first, 1),
            uploadOf(SECOND_TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, SECOND_TEXTURE_ID, &other, 2),
            uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &reloadedUser, 3, {TEXTURE_SLOT, TEXTURE_SLOT}),
            uploadOf(SECOND_MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, SECOND_MATERIAL_ID, &otherUser, 4, {SECOND_TEXTURE_SLOT}),
        };
        copies.apply(load, FIRST_FRAME, finished);
        oldHandle = device.getBindlessHandle(copies.findCopy(TEXTURE_SLOT)->texture);
        otherHandle = device.getBindlessHandle(copies.findCopy(SECOND_TEXTURE_SLOT)->texture);
        const std::vector<ResourceUploadRecord> reload{uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &reloaded, 5)};

        copies.apply(reload, FIRST_FRAME, finished);

        newHandle = device.getBindlessHandle(copies.findCopy(TEXTURE_SLOT)->texture);
        reloadedRecord = copies.getMaterials().getRecord(copies.findCopy(MATERIAL_SLOT)->materialRecord);
        otherRecord = copies.getMaterials().getRecord(copies.findCopy(SECOND_MATERIAL_SLOT)->materialRecord);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(newHandle != 0);
    CHECK(newHandle != oldHandle);
    CHECK(handleAt(reloadedRecord, BASE_COLOR_INPUT) == newHandle);
    CHECK(handleAt(reloadedRecord, EMISSIVE_INPUT) == newHandle);
    CHECK(handleAt(reloadedRecord, PACKED_ORM_INPUT) == 0);
    CHECK(handleAt(otherRecord, BASE_COLOR_INPUT) == otherHandle);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("record 0 holds the default material from initialize, and no material is given it", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    const Material material = makeMaterial();
    GpuMaterial defaultRecord;
    std::uint32_t materialRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    bool bufferCreated = false;
    const GpuCopiesRun run = runWithCopies("GpuCopies default material test", [&](TechEngine::RenderDevice&, GpuCopies& copies) {
        bufferCreated = copies.getMaterials().getBuffer().valid();
        const std::vector<ResourceUploadRecord> records{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 1)};
        std::vector<FinishedUpload> finished;

        copies.apply(records, FIRST_FRAME, finished);

        defaultRecord = copies.getMaterials().getRecord(TechEngine::DEFAULT_MATERIAL_RECORD);
        materialRecord = copies.findCopy(MATERIAL_SLOT)->materialRecord;
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(bufferCreated);
    CHECK(defaultRecord == TechEngine::MaterialBuffer::getDefaultMaterial());
    CHECK(defaultRecord.albedo == Vec4{1.0F, 1.0F, 1.0F, 1.0F});
    CHECK(defaultRecord.roughnessFactor == 1.0F);
    CHECK(defaultRecord.metallicFactor == 0.0F);
    CHECK(materialRecord != TechEngine::DEFAULT_MATERIAL_RECORD);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("a material reload keeps its record and writes the new values over it", "[client][render][upload][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    const Material first = makeMaterial();
    Material reloaded = makeMaterial();
    reloaded.baseColorFactor = Vec4{0.0F, 1.0F, 0.0F, 1.0F};
    reloaded.roughnessFactor = 0.125F;
    std::uint32_t firstRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    std::uint32_t reloadedRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    std::uint32_t nextRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    GpuMaterial record;
    const GpuCopiesRun run = runWithCopies("GpuCopies material reload test", [&](TechEngine::RenderDevice&, GpuCopies& copies) {
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &first, 1)};
        copies.apply(load, FIRST_FRAME, finished);
        firstRecord = copies.findCopy(MATERIAL_SLOT)->materialRecord;
        const std::vector<ResourceUploadRecord> reload{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &reloaded, 2)};

        copies.apply(reload, FIRST_FRAME, finished);
        const std::vector<ResourceUploadRecord> another{uploadOf(SECOND_MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, SECOND_MATERIAL_ID, &first, 3)};
        copies.apply(another, FIRST_FRAME, finished);

        reloadedRecord = copies.findCopy(MATERIAL_SLOT)->materialRecord;
        nextRecord = copies.findCopy(SECOND_MATERIAL_SLOT)->materialRecord;
        record = copies.getMaterials().getRecord(reloadedRecord);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(firstRecord != TechEngine::DEFAULT_MATERIAL_RECORD);
    CHECK(reloadedRecord == firstRecord);
    CHECK(nextRecord == firstRecord + 1);
    CHECK(record.albedo == reloaded.baseColorFactor);
    CHECK(record.roughnessFactor == reloaded.roughnessFactor);
    CHECK(run.glError == GL_NO_ERROR);
}

static Mesh makeTriangle() {
    Mesh mesh;
    mesh.positions = {Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F}};
    mesh.indices = {0, 1, 2};
    return mesh;
}

TEST_CASE("a dropped mesh frees its range, so the next mesh of the same size is placed there", "[client][render][upload][drop][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    const Mesh triangle = makeTriangle();
    static constexpr ResourceSlotId EARLY_SLOT{7, 0};
    static constexpr ResourceSlotId LATE_SLOT{8, 0};
    GeometryRange dropped;
    GeometryRange kept;
    GeometryRange beforeDrop;
    GeometryRange afterDrop;
    const GpuCopiesRun run = runWithCopies("GpuCopies mesh drop test", [&](TechEngine::RenderDevice&, GpuCopies& copies) {
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &triangle, 1),
            uploadOf(SECOND_MESH_SLOT, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &triangle, 2),
        };
        copies.apply(load, FIRST_FRAME, finished);
        dropped = copies.findCopy(MESH_SLOT)->geometry;
        kept = copies.findCopy(SECOND_MESH_SLOT)->geometry;
        const std::vector<ResourceUploadRecord> release{releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID)};
        copies.apply(release, 2, finished);
        copies.retireUnreachable(NO_PACKETS, 2);

        const std::vector<ResourceUploadRecord> early{uploadOf(EARLY_SLOT, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &triangle, 3)};
        copies.apply(early, 2, finished);
        beforeDrop = copies.findCopy(EARLY_SLOT)->geometry;
        copies.dropFinished(1);
        const std::vector<ResourceUploadRecord> late{uploadOf(LATE_SLOT, TechEngine::MESH_TYPE_TAG, UNRELATED_ID, &triangle, 4)};
        copies.apply(late, 2, finished);
        afterDrop = copies.findCopy(LATE_SLOT)->geometry;
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(dropped.vertexCount == 3);
    CHECK(kept.baseVertex != dropped.baseVertex);
    CHECK(beforeDrop.baseVertex != dropped.baseVertex);
    CHECK(afterDrop == dropped);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("a mesh's default material and that material's texture stay while a packet draws the mesh, and retire with it", "[client][render][upload][drop][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    Mesh mesh;
    mesh.slots = {MeshSlot{"slot", 0, 0, MATERIAL_ID}};
    Material material = makeMaterial();
    material.baseColorTexture.id = TEXTURE_ID;
    const Material later = makeMaterial();
    bool chainKept = false;
    bool chainGone = false;
    std::size_t retiredWhileDrawn = 99;
    std::size_t retiredAfter = 0;
    std::size_t liveBeforeDrop = 0;
    std::size_t liveAfterDrop = 0;
    std::uint32_t droppedRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    std::uint32_t reusedRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    const GpuCopiesRun run = runWithCopies("GpuCopies dependency drop test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        const Texture texture = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &texture, 1),
            uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 2, {TEXTURE_SLOT}),
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 3, {MATERIAL_SLOT}),
        };
        copies.apply(load, FIRST_FRAME, finished);
        droppedRecord = copies.findCopy(MATERIAL_SLOT)->materialRecord;
        const std::vector<ResourceUploadRecord> releases{
            releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
            releaseOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID),
            releaseOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID),
        };
        copies.apply(releases, 2, finished);
        const FramePacket drawing = packetDrawing(MESH_SLOT);

        copies.retireUnreachable(std::array<const FramePacket*, 2>{nullptr, &drawing}, 2);

        chainKept = copies.findCopy(MESH_SLOT) != nullptr && copies.findCopy(MATERIAL_SLOT) != nullptr && copies.findCopy(TEXTURE_SLOT) != nullptr;
        retiredWhileDrawn = copies.getRetiredCount();
        copies.retireUnreachable(NO_PACKETS, 3);
        chainGone = copies.findCopy(MESH_SLOT) == nullptr && copies.findCopy(MATERIAL_SLOT) == nullptr && copies.findCopy(TEXTURE_SLOT) == nullptr;
        retiredAfter = copies.getRetiredCount();
        liveBeforeDrop = device.getLiveObjectCount();
        copies.dropFinished(2);
        liveAfterDrop = device.getLiveObjectCount();
        const std::vector<ResourceUploadRecord> another{uploadOf(SECOND_MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, SECOND_MATERIAL_ID, &later, 4)};
        copies.apply(another, 3, finished);
        reusedRecord = copies.findCopy(SECOND_MATERIAL_SLOT)->materialRecord;
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(chainKept);
    CHECK(retiredWhileDrawn == 0);
    CHECK(chainGone);
    CHECK(retiredAfter == 3);
    CHECK(liveAfterDrop == liveBeforeDrop - 1);
    CHECK(droppedRecord != TechEngine::DEFAULT_MATERIAL_RECORD);
    CHECK(reusedRecord == droppedRecord);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("a texture reload destroys the old texture only once the frame before the reload has finished", "[client][render][upload][drop][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    std::size_t afterLoad = 0;
    std::size_t afterReload = 0;
    std::size_t afterEarlyDrop = 0;
    std::size_t afterDrop = 0;
    const GpuCopiesRun run = runWithCopies("GpuCopies texture drop test", [&](TechEngine::RenderDevice& device, GpuCopies& copies) {
        const Texture first = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        const Texture reloaded = makeTexture(TextureFormat::Bc7Srgb, 8, 8);
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &first, 1)};
        copies.apply(load, FIRST_FRAME, finished);
        afterLoad = device.getLiveObjectCount();
        const std::vector<ResourceUploadRecord> reload{uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &reloaded, 2)};

        copies.apply(reload, 3, finished);

        afterReload = device.getLiveObjectCount();
        copies.dropFinished(1);
        afterEarlyDrop = device.getLiveObjectCount();
        copies.dropFinished(2);
        afterDrop = device.getLiveObjectCount();
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(afterReload == afterLoad + 1);
    CHECK(afterEarlyDrop == afterReload);
    CHECK(afterDrop == afterLoad);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("dropping a material reload's old copy leaves the record the new copy kept", "[client][render][upload][drop][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    const Material first = makeMaterial();
    Material reloaded = makeMaterial();
    reloaded.baseColorFactor = Vec4{0.0F, 1.0F, 0.0F, 1.0F};
    std::uint32_t keptRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    std::uint32_t nextRecord = TechEngine::DEFAULT_MATERIAL_RECORD;
    std::size_t retiredAfterDrop = 99;
    GpuMaterial record;
    const GpuCopiesRun run = runWithCopies("GpuCopies material drop test", [&](TechEngine::RenderDevice&, GpuCopies& copies) {
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> load{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &first, 1)};
        copies.apply(load, FIRST_FRAME, finished);
        const std::vector<ResourceUploadRecord> reload{uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &reloaded, 2)};
        copies.apply(reload, 2, finished);
        keptRecord = copies.findCopy(MATERIAL_SLOT)->materialRecord;

        copies.dropFinished(1);

        retiredAfterDrop = copies.getRetiredCount();
        const std::vector<ResourceUploadRecord> another{uploadOf(SECOND_MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, SECOND_MATERIAL_ID, &first, 3)};
        copies.apply(another, 2, finished);
        nextRecord = copies.findCopy(SECOND_MATERIAL_SLOT)->materialRecord;
        record = copies.getMaterials().getRecord(keptRecord);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initializeError == std::error_code{});
    CHECK(retiredAfterDrop == 0);
    CHECK(keptRecord != TechEngine::DEFAULT_MATERIAL_RECORD);
    CHECK(nextRecord != keptRecord);
    CHECK(record.albedo == reloaded.baseColorFactor);
    CHECK(run.glError == GL_NO_ERROR);
}

TEST_CASE("shutdown destroys the textures of retired copies too", "[client][render][upload][drop][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "GpuCopies retired shutdown test") == std::error_code{});
    std::error_code initializeError;
    std::size_t retired = 0;
    std::size_t liveAfterShutdown = 99;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        GpuCopies copies;
        initializeError = copies.initialize(device);
        const Texture first = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        const Texture reloaded = makeTexture(TextureFormat::Bc7Srgb, 4, 4);
        std::vector<FinishedUpload> finished;
        const std::vector<ResourceUploadRecord> records{
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &first, 1),
            uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &reloaded, 2),
        };
        copies.apply(records, FIRST_FRAME, finished);
        retired = copies.getRetiredCount();

        copies.shutdown();

        liveAfterShutdown = device.getLiveObjectCount();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(initializeError == std::error_code{});
    CHECK(retired == 1);
    CHECK(liveAfterShutdown == 0);
    CHECK(error == GL_NO_ERROR);
}
