#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/FrameRenderer.hpp>
#include <render/FrameView.hpp>
#include <render/GpuCopies.hpp>
#include <render/device/RenderDevice.hpp>
#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>
#include <render/device/target/RenderTarget.hpp>
#include <render/device/texture/Texture.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

using TechEngine::FramePacket;
using TechEngine::FrameView;
using TechEngine::Mat4;
using TechEngine::Material;
using TechEngine::MaterialAlphaMode;
using TechEngine::Mesh;
using TechEngine::MeshSlot;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceUploadKind;
using TechEngine::ResourceUploadRecord;
using TechEngine::Vec3;
using TechEngine::Vec4;

enum class FrameShaderFailure { VertexCompile, FragmentCompile, Link };

static PFNGLSHADERSOURCEPROC g_originalShaderSource = nullptr;
static FrameShaderFailure g_shaderFailure = FrameShaderFailure::VertexCompile;

static void GLAD_API_PTR failingShaderSource(unsigned int shader, int count, const char* const* source, const int* length) {
    int type = 0;
    glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    const char* replacement = nullptr;
    if ((g_shaderFailure == FrameShaderFailure::VertexCompile && type == GL_VERTEX_SHADER) || (g_shaderFailure == FrameShaderFailure::FragmentCompile && type == GL_FRAGMENT_SHADER)) {
        replacement = "#version 460 core\ninvalid shader source";
    } else if (g_shaderFailure == FrameShaderFailure::Link) {
        replacement = type == GL_VERTEX_SHADER ? "#version 460 core\nout vec3 mismatch; void main() { mismatch = vec3(1.0); gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }" : "#version 450 core\nin vec4 mismatch; out vec4 color; void main() { color = mismatch; }";
    }
    if (replacement != nullptr) {
        g_originalShaderSource(shader, 1, &replacement, nullptr);
    } else {
        g_originalShaderSource(shader, count, source, length);
    }
}

struct FrameShaderFailureScope {
    explicit FrameShaderFailureScope(FrameShaderFailure failure) {
        g_shaderFailure = failure;
        g_originalShaderSource = glad_glShaderSource;
        glad_glShaderSource = failingShaderSource;
    }

    ~FrameShaderFailureScope() {
        glad_glShaderSource = g_originalShaderSource;
    }
};

static constexpr int IMAGE_SIZE = 64;
using Image = std::array<std::byte, static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE * 4>;

static constexpr ResourceSlotId MESH_SLOT{1, 0};
static constexpr ResourceSlotId GREEN_SLOT{2, 0};
static constexpr ResourceSlotId DISCARDED_CUTOUT_SLOT{3, 0};
static constexpr ResourceSlotId KEPT_CUTOUT_SLOT{4, 0};
static constexpr ResourceId MESH_ID{0x6001};
static constexpr ResourceId GREEN_ID{0x6002};
static constexpr ResourceId DISCARDED_CUTOUT_ID{0x6003};
static constexpr ResourceId KEPT_CUTOUT_ID{0x6004};

static constexpr std::array<float, 4> RED_CLEAR{1.0F, 0.0F, 0.0F, 1.0F};

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

// Each face is wound counter-clockwise as seen from the origin, so a camera there fills the image with front faces whichever axis is its forward.
static void addInwardFace(Mesh& mesh, const Vec3& forward, const Vec3& up) {
    constexpr float HALF_SIZE = 3.0f;
    const Vec3 right = glm::cross(forward, up);
    const Vec3 center = forward * HALF_SIZE;
    const auto base = static_cast<std::uint32_t>(mesh.positions.size());
    mesh.positions.push_back(center + (-right - up) * HALF_SIZE);
    mesh.positions.push_back(center + (right - up) * HALF_SIZE);
    mesh.positions.push_back(center + (right + up) * HALF_SIZE);
    mesh.positions.push_back(center + (-right + up) * HALF_SIZE);
    for (const std::uint32_t corner: {0U, 1U, 2U, 0U, 2U, 3U}) {
        mesh.indices.push_back(base + corner);
    }
}

static Mesh makeInwardCube(const ResourceId defaultMaterial) {
    Mesh mesh;
    mesh.id = MESH_ID;
    addInwardFace(mesh, Vec3{0.0f, 0.0f, -1.0f}, Vec3{0.0f, 1.0f, 0.0f});
    addInwardFace(mesh, Vec3{0.0f, 0.0f, 1.0f}, Vec3{0.0f, 1.0f, 0.0f});
    addInwardFace(mesh, Vec3{-1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    addInwardFace(mesh, Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    addInwardFace(mesh, Vec3{0.0f, -1.0f, 0.0f}, Vec3{0.0f, 0.0f, -1.0f});
    addInwardFace(mesh, Vec3{0.0f, 1.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f});
    mesh.slots.push_back(MeshSlot{"cube", 0, static_cast<std::uint32_t>(mesh.indices.size()), defaultMaterial});
    return mesh;
}

static Material makeMaterial(const ResourceId id, const Vec4& baseColorFactor, const MaterialAlphaMode alphaMode = MaterialAlphaMode::Opaque, const float alphaCutoff = 0.5f) {
    Material material;
    material.id = id;
    material.baseColorFactor = baseColorFactor;
    material.alphaMode = alphaMode;
    material.alphaCutoff = alphaCutoff;
    return material;
}

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

static ResourceHandle<Material> materialHandle(const ResourceSlotId slot) {
    return ResourceHandle<Material>{slot.index, slot.generation};
}

// The view points into packet, so both stay where the caller declared them.
static void fillFrame(FramePacket& packet, FrameView& view, const Mat4& world, const std::vector<ResourceHandle<Material>>& materials, const bool hasCamera) {
    packet.instances.clear();
    packet.materials = materials;
    packet.instances.push_back(TechEngine::PacketInstance{
        .world = world,
        .mesh = ResourceHandle<Mesh>{MESH_SLOT.index, MESH_SLOT.generation},
        .firstMaterial = 0,
        .materialCount = static_cast<std::uint32_t>(materials.size()),
    });
    view = FrameView{};
    view.packet = &packet;
    view.clearColor = RED_CLEAR;
    view.instanceWorlds = {world};
    view.hasCamera = hasCamera;
    view.camera.settings.verticalFieldOfView = 90.0f;
    view.camera.settings.nearPlane = 0.1f;
    view.camera.settings.farPlane = 10.0f;
}

static void drawOnce(TechEngine::FrameRenderer& renderer, const FrameView& view, const TechEngine::GpuCopies& copies, const TechEngine::RenderTargetHandle target, const TechEngine::RenderExtent& extent) {
    renderer.prepare(view, copies);
    const TechEngine::PacketCamera* camera = view.hasCamera ? &view.camera : nullptr;
    renderer.draw(view, copies, TechEngine::ViewDraw{target, extent, camera});
}

static std::size_t countPixels(const Image& image, const unsigned char red, const unsigned char green, const unsigned char blue, const int width = IMAGE_SIZE, const int height = IMAGE_SIZE) {
    std::size_t count = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const std::size_t i = (static_cast<std::size_t>(y) * IMAGE_SIZE + static_cast<std::size_t>(x)) * 4;
            const auto channel = [&](const std::size_t at) {
                return static_cast<unsigned char>(image[at]);
            };
            if (channel(i) == red && channel(i + 1) == green && channel(i + 2) == blue && channel(i + 3) == 255) {
                count++;
            }
        }
    }
    return count;
}

struct ForwardRun {
    bool loaded = false;
    std::error_code copiesError;
    bool initialized = false;
    unsigned int error = GL_NO_ERROR;
};

template<typename Body>
static ForwardRun runForward(const char* title, Body&& body) {
    TechEngine::Window window;
    ForwardRun run;
    REQUIRE(window.open(320, 240, title) == std::error_code{});
    run.loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        TechEngine::GpuCopies copies;
        TechEngine::FrameRenderer renderer;
        TechEngine::TextureHandle color;
        TechEngine::TextureHandle depth;
        TechEngine::RenderTargetHandle target;
        run.copiesError = copies.initialize(device);
        const std::error_code colorError = device.createTexture({IMAGE_SIZE, IMAGE_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, color);
        const std::error_code depthError = device.createTexture({IMAGE_SIZE, IMAGE_SIZE, 1, TechEngine::GpuTextureFormat::Depth32F}, depth);
        const bool texturesCreated = !colorError && !depthError;
        const bool targetCreated = texturesCreated && !device.createRenderTarget({color, depth}, target);
        run.initialized = !run.copiesError && targetCreated && renderer.initialize(device);
        if (run.initialized) {
            body(device, copies, renderer, color, target);
        }
        renderer.shutdown();
        copies.shutdown();
        device.shutdown();
        run.error = glGetError();
    });
    return run;
}

TEST_CASE("Frame renderer rejects shader failures and can retry initialization", "[client][render][window][gpu]") {
    FrameShaderFailure failure = FrameShaderFailure::VertexCompile;
    SECTION("Vertex compilation fails") {
        failure = FrameShaderFailure::VertexCompile;
    }
    SECTION("Fragment compilation fails") {
        failure = FrameShaderFailure::FragmentCompile;
    }
    SECTION("Shader interfaces fail to link") {
        failure = FrameShaderFailure::Link;
    }

    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Shader failure test") == std::error_code{});
    bool loaded = false;
    bool rejected = false;
    bool recovered = false;
    std::size_t leftAfterRejection = 0;
    std::size_t leftAfterShutdown = 0;
    unsigned int error = GL_NO_ERROR;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0;
            if (loaded) {
                TechEngine::RenderDevice device;
                TechEngine::FrameRenderer renderer;
                {
                    const FrameShaderFailureScope failureScope{failure};
                    rejected = !renderer.initialize(device);
                }
                leftAfterRejection = device.getLiveObjectCount();
                recovered = renderer.initialize(device);
                renderer.shutdown();
                renderer.shutdown();
                leftAfterShutdown = device.getLiveObjectCount();
                device.shutdown();
                error = glGetError();
            }
            window.releaseContext();
        });
    }
    REQUIRE(loaded);
    CHECK(rejected);
    CHECK(leftAfterRejection == 0);
    CHECK(recovered);
    CHECK(leftAfterShutdown == 0);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("the forward pass draws base color times its factor through the main camera, and only clears without one", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});

    constexpr int SMALL_WIDTH = 32;
    constexpr int SMALL_HEIGHT = 48;
    Image noCamera{};
    Image drawn{};
    Image repeated{};
    Image small{};
    Image minimized{};
    const Mesh cube = makeInwardCube(ResourceId{});
    const Material green = makeMaterial(GREEN_ID, Vec4{0.0f, 1.0f, 0.0f, 1.0f});
    const ForwardRun run = runForward("Forward pass test", [&](TechEngine::RenderDevice& device, TechEngine::GpuCopies& copies, TechEngine::FrameRenderer& renderer, const TechEngine::TextureHandle color, const TechEngine::RenderTargetHandle target) {
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &cube, 1),
            uploadOf(GREEN_SLOT, TechEngine::MATERIAL_TYPE_TAG, GREEN_ID, &green, 2),
        };
        std::vector<TechEngine::FinishedUpload> finished;
        copies.apply(records, finished);

        const TechEngine::RenderExtent full{IMAGE_SIZE, IMAGE_SIZE};
        FramePacket packet;
        FrameView view;
        fillFrame(packet, view, Mat4(1.0f), {materialHandle(GREEN_SLOT)}, false);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, noCamera);

        fillFrame(packet, view, Mat4(1.0f), {materialHandle(GREEN_SLOT)}, true);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, drawn);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, repeated);

        FrameView clearOnly;
        FramePacket clearPacket;
        fillFrame(clearPacket, clearOnly, Mat4(1.0f), {}, false);
        drawOnce(renderer, clearOnly, copies, target, full);
        drawOnce(renderer, view, copies, target, TechEngine::RenderExtent{SMALL_WIDTH, SMALL_HEIGHT});
        device.readTexture(color, small);

        drawOnce(renderer, view, copies, target, TechEngine::RenderExtent{0, SMALL_HEIGHT});
        drawOnce(renderer, view, copies, target, TechEngine::RenderExtent{SMALL_WIDTH, 0});
        device.readTexture(color, minimized);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initialized);
    CHECK(run.error == GL_NO_ERROR);
    const std::size_t total = static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE;
    const std::size_t smallArea = static_cast<std::size_t>(SMALL_WIDTH) * SMALL_HEIGHT;
    CHECK(countPixels(noCamera, 255, 0, 0) == total);
    CHECK(countPixels(drawn, 0, 255, 0) == total);
    const bool repeatedMatches = repeated == drawn;
    CHECK(repeatedMatches);
    CHECK(countPixels(small, 0, 255, 0, SMALL_WIDTH, SMALL_HEIGHT) == smallArea);
    CHECK(countPixels(small, 255, 0, 0) == total - smallArea);
    const bool minimizedMatches = minimized == small;
    CHECK(minimizedMatches);
}

TEST_CASE("a mirrored instance of a single-sided mesh is drawn, not culled", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});

    Image mirrored{};
    const Mesh cube = makeInwardCube(ResourceId{});
    const Material green = makeMaterial(GREEN_ID, Vec4{0.0f, 1.0f, 0.0f, 1.0f});
    const ForwardRun run = runForward("Mirrored instance test", [&](TechEngine::RenderDevice& device, TechEngine::GpuCopies& copies, TechEngine::FrameRenderer& renderer, const TechEngine::TextureHandle color, const TechEngine::RenderTargetHandle target) {
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &cube, 1),
            uploadOf(GREEN_SLOT, TechEngine::MATERIAL_TYPE_TAG, GREEN_ID, &green, 2),
        };
        std::vector<TechEngine::FinishedUpload> finished;
        copies.apply(records, finished);

        FramePacket packet;
        FrameView view;
        fillFrame(packet, view, glm::scale(Mat4(1.0f), Vec3{-1.0f, 1.0f, 1.0f}), {materialHandle(GREEN_SLOT)}, true);
        drawOnce(renderer, view, copies, target, TechEngine::RenderExtent{IMAGE_SIZE, IMAGE_SIZE});
        device.readTexture(color, mirrored);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initialized);
    CHECK(run.error == GL_NO_ERROR);
    CHECK(countPixels(mirrored, 0, 255, 0) == static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE);
}

TEST_CASE("a cut-out material discards a texel whose alpha is under its cutoff and keeps one over it", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});

    Image discarded{};
    Image kept{};
    const Mesh cube = makeInwardCube(ResourceId{});
    const Material under = makeMaterial(DISCARDED_CUTOUT_ID, Vec4{0.0f, 1.0f, 0.0f, 0.2f}, MaterialAlphaMode::Cutout, 0.5f);
    const Material over = makeMaterial(KEPT_CUTOUT_ID, Vec4{0.0f, 1.0f, 0.0f, 0.75f}, MaterialAlphaMode::Cutout, 0.5f);
    const ForwardRun run = runForward("Cut-out test", [&](TechEngine::RenderDevice& device, TechEngine::GpuCopies& copies, TechEngine::FrameRenderer& renderer, const TechEngine::TextureHandle color, const TechEngine::RenderTargetHandle target) {
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &cube, 1),
            uploadOf(DISCARDED_CUTOUT_SLOT, TechEngine::MATERIAL_TYPE_TAG, DISCARDED_CUTOUT_ID, &under, 2),
            uploadOf(KEPT_CUTOUT_SLOT, TechEngine::MATERIAL_TYPE_TAG, KEPT_CUTOUT_ID, &over, 3),
        };
        std::vector<TechEngine::FinishedUpload> finished;
        copies.apply(records, finished);

        const TechEngine::RenderExtent full{IMAGE_SIZE, IMAGE_SIZE};
        FramePacket packet;
        FrameView view;
        fillFrame(packet, view, Mat4(1.0f), {materialHandle(DISCARDED_CUTOUT_SLOT)}, true);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, discarded);
        fillFrame(packet, view, Mat4(1.0f), {materialHandle(KEPT_CUTOUT_SLOT)}, true);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, kept);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initialized);
    CHECK(run.error == GL_NO_ERROR);
    const std::size_t total = static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE;
    CHECK(countPixels(discarded, 255, 0, 0) == total);
    CHECK(countPixels(kept, 0, 255, 0) == total);
}

TEST_CASE("a null slot material draws record 0's white, and a slot past the instance's materials draws the mesh slot's default", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});

    Image nullMaterial{};
    Image meshDefault{};
    const Mesh cube = makeInwardCube(GREEN_ID);
    const Material green = makeMaterial(GREEN_ID, Vec4{0.0f, 1.0f, 0.0f, 1.0f});
    const ForwardRun run = runForward("Slot fallback test", [&](TechEngine::RenderDevice& device, TechEngine::GpuCopies& copies, TechEngine::FrameRenderer& renderer, const TechEngine::TextureHandle color, const TechEngine::RenderTargetHandle target) {
        const std::vector<ResourceUploadRecord> records{
            uploadOf(GREEN_SLOT, TechEngine::MATERIAL_TYPE_TAG, GREEN_ID, &green, 1),
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &cube, 2, {GREEN_SLOT}),
        };
        std::vector<TechEngine::FinishedUpload> finished;
        copies.apply(records, finished);

        const TechEngine::RenderExtent full{IMAGE_SIZE, IMAGE_SIZE};
        FramePacket packet;
        FrameView view;
        fillFrame(packet, view, Mat4(1.0f), {ResourceHandle<Material>{}}, true);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, nullMaterial);
        fillFrame(packet, view, Mat4(1.0f), {}, true);
        drawOnce(renderer, view, copies, target, full);
        device.readTexture(color, meshDefault);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initialized);
    CHECK(run.error == GL_NO_ERROR);
    const std::size_t total = static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE;
    CHECK(countPixels(nullMaterial, 255, 255, 255) == total);
    CHECK(countPixels(meshDefault, 0, 255, 0) == total);
}

TEST_CASE("one prepared frame draws each view through its own camera and size, and a view with no camera only clears", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});

    constexpr int SMALL_WIDTH = 32;
    constexpr int SMALL_HEIGHT = 48;
    Image first{};
    Image firstAfterSecond{};
    Image second{};
    Image clipped{};
    Image cleared{};
    bool secondTargetCreated = false;
    const Mesh cube = makeInwardCube(ResourceId{});
    const Material green = makeMaterial(GREEN_ID, Vec4{0.0f, 1.0f, 0.0f, 1.0f});
    const ForwardRun run = runForward("Two views test", [&](TechEngine::RenderDevice& device, TechEngine::GpuCopies& copies, TechEngine::FrameRenderer& renderer, const TechEngine::TextureHandle color, const TechEngine::RenderTargetHandle target) {
        const std::vector<ResourceUploadRecord> records{
            uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &cube, 1),
            uploadOf(GREEN_SLOT, TechEngine::MATERIAL_TYPE_TAG, GREEN_ID, &green, 2),
        };
        std::vector<TechEngine::FinishedUpload> finished;
        copies.apply(records, finished);

        TechEngine::TextureHandle secondColor;
        TechEngine::TextureHandle secondDepth;
        TechEngine::RenderTargetHandle secondTarget;
        const std::error_code colorError = device.createTexture({IMAGE_SIZE, IMAGE_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, secondColor);
        const std::error_code depthError = device.createTexture({IMAGE_SIZE, IMAGE_SIZE, 1, TechEngine::GpuTextureFormat::Depth32F}, secondDepth);
        secondTargetCreated = !colorError && !depthError && !device.createRenderTarget({secondColor, secondDepth}, secondTarget);
        if (!secondTargetCreated) {
            return;
        }

        FramePacket packet;
        FrameView view;
        fillFrame(packet, view, Mat4(1.0f), {materialHandle(GREEN_SLOT)}, true);
        TechEngine::PacketCamera shortSighted = view.camera;
        shortSighted.settings.farPlane = 1.0f;

        renderer.prepare(view, copies);
        renderer.draw(view, copies, TechEngine::ViewDraw{target, TechEngine::RenderExtent{IMAGE_SIZE, IMAGE_SIZE}, &view.camera});
        device.readTexture(color, first);
        renderer.draw(view, copies, TechEngine::ViewDraw{secondTarget, TechEngine::RenderExtent{SMALL_WIDTH, SMALL_HEIGHT}, &view.camera});
        device.readTexture(secondColor, second);
        device.readTexture(color, firstAfterSecond);
        renderer.draw(view, copies, TechEngine::ViewDraw{secondTarget, TechEngine::RenderExtent{IMAGE_SIZE, IMAGE_SIZE}, &shortSighted});
        device.readTexture(secondColor, clipped);
        renderer.draw(view, copies, TechEngine::ViewDraw{secondTarget, TechEngine::RenderExtent{IMAGE_SIZE, IMAGE_SIZE}, nullptr});
        device.readTexture(secondColor, cleared);

        device.destroyRenderTarget(secondTarget);
        device.destroyTexture(secondDepth);
        device.destroyTexture(secondColor);
    });

    REQUIRE(run.loaded);
    REQUIRE(run.initialized);
    REQUIRE(secondTargetCreated);
    CHECK(run.error == GL_NO_ERROR);
    const std::size_t total = static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE;
    const std::size_t smallArea = static_cast<std::size_t>(SMALL_WIDTH) * SMALL_HEIGHT;
    CHECK(countPixels(first, 0, 255, 0) == total);
    const bool firstKept = firstAfterSecond == first;
    CHECK(firstKept);
    CHECK(countPixels(second, 0, 255, 0, SMALL_WIDTH, SMALL_HEIGHT) == smallArea);
    CHECK(countPixels(clipped, 255, 0, 0) == total);
    CHECK(countPixels(cleared, 255, 0, 0) == total);
}
