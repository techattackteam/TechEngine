#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <render/GpuCopies.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <utility>
#include <vector>

using TechEngine::FinishedUpload;
using TechEngine::GeometryRange;
using TechEngine::GpuCopies;
using TechEngine::GpuCopy;
using TechEngine::Material;
using TechEngine::Mesh;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceUploadKind;
using TechEngine::ResourceUploadRecord;
using TechEngine::Texture;

static constexpr ResourceId MESH_ID{0x5001};
static constexpr ResourceId MATERIAL_ID{0x5002};
static constexpr ResourceId TEXTURE_ID{0x5003};
static constexpr ResourceId UNRELATED_ID{0x5999};

static constexpr ResourceSlotId MESH_SLOT{1, 0};
static constexpr ResourceSlotId MATERIAL_SLOT{2, 0};
static constexpr ResourceSlotId TEXTURE_SLOT{3, 0};

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

    copies.apply(records, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 7}});
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->typeTag == TechEngine::MESH_TYPE_TAG);
    REQUIRE(copy->id == MESH_ID);
    REQUIRE(copy->dependencies == std::vector<ResourceSlotId>{MATERIAL_SLOT});
    REQUIRE_FALSE(copy->released);
}

TEST_CASE("a dependency is found through the upload's slot and the label through its id, never through the payload's ids", "[client][render][upload]") {
    GpuCopies copies;
    const Texture texture{};
    Material material;
    material.id = UNRELATED_ID;
    material.baseColorTexture.id = UNRELATED_ID;
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MATERIAL_SLOT, TechEngine::MATERIAL_TYPE_TAG, MATERIAL_ID, &material, 1, {TEXTURE_SLOT}),
        uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &texture, 2),
    };
    std::vector<FinishedUpload> finished;

    copies.apply(records, finished);

    const GpuCopy* materialCopy = copies.findCopy(MATERIAL_SLOT);
    REQUIRE(materialCopy != nullptr);
    REQUIRE(materialCopy->id == MATERIAL_ID);
    REQUIRE(materialCopy->dependencies == std::vector<ResourceSlotId>{TEXTURE_SLOT});
    const GpuCopy* textureCopy = copies.findCopy(materialCopy->dependencies[0]);
    REQUIRE(textureCopy != nullptr);
    REQUIRE(textureCopy->id == TEXTURE_ID);
}

TEST_CASE("tokens are reported in record order, one per upload and none for a release", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh mesh{};
    const Texture texture{};
    const std::vector<ResourceUploadRecord> records{
        uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &mesh, 4),
        releaseOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID),
        uploadOf(TEXTURE_SLOT, TechEngine::TEXTURE_TYPE_TAG, TEXTURE_ID, &texture, 9),
    };
    std::vector<FinishedUpload> finished;

    copies.apply(records, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 4}, FinishedUpload{TEXTURE_SLOT, 9}});
}

TEST_CASE("a second upload to the same slot replaces the copy and reports its own slot and token", "[client][render][upload]") {
    GpuCopies copies;
    const Mesh first{};
    const Mesh second{};
    std::vector<FinishedUpload> finished;
    const std::vector<ResourceUploadRecord> load{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &first, 1, {MATERIAL_SLOT})};
    copies.apply(load, finished);

    const std::vector<ResourceUploadRecord> reload{uploadOf(MESH_SLOT, TechEngine::MESH_TYPE_TAG, MESH_ID, &second, 2, {TEXTURE_SLOT})};
    copies.apply(reload, finished);

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

    copies.apply(records, finished);

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

    copies.apply(records, finished);

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

    copies.apply(records, finished);

    REQUIRE(finished == std::vector<FinishedUpload>{FinishedUpload{MESH_SLOT, 3}});
    const GpuCopy* copy = copies.findCopy(MESH_SLOT);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->geometry == GeometryRange{});
}

TEST_CASE("a slot that never had an upload has no copy", "[client][render][upload]") {
    GpuCopies copies;

    REQUIRE(copies.findCopy(MESH_SLOT) == nullptr);
    REQUIRE(copies.findCopy(ResourceSlotId{}) == nullptr);
}
