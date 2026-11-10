#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::ImportBlock;
using TechEngine::ImportSettings;
using TechEngine::Mesh;
using TechEngine::MeshSlot;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::UpAxis;
using TechEngine::Vec2;
using TechEngine::Vec3;
using TechEngine::Vec4;
using TechEngine::Writer;

static constexpr ResourceId MESH_ASSET_ID{0x6D65736800000001};
static constexpr ResourceId STONE_MATERIAL_ID{0x6D61740000000001};
static constexpr ResourceId GLASS_MATERIAL_ID{0x6D61740000000002};

static Mesh makeTriangle(const ResourceId id = MESH_ASSET_ID) {
    Mesh mesh;
    mesh.id = id;
    mesh.positions = {Vec3(-0.5F, -0.5F, 0.0F), Vec3(0.5F, -0.5F, 0.0F), Vec3(0.0F, 0.5F, 0.0F)};
    mesh.indices = {0, 1, 2};
    mesh.slots = {MeshSlot{"Slot 0", 0, 3, ResourceId{}}};
    return mesh;
}

// A quad in one slot and a triangle in a second, with every stream.
static Mesh makeFullMesh() {
    Mesh mesh;
    mesh.id = MESH_ASSET_ID;
    mesh.importBlock = ImportBlock{"assets/client/Ship.gltf", 4, "Hull", ImportSettings{0.01F, UpAxis::Z}};
    mesh.positions = {Vec3(0.0F, 0.0F, 0.0F), Vec3(1.0F, 0.0F, 0.0F), Vec3(1.0F, 1.0F, 0.0F), Vec3(0.0F, 1.0F, 0.0F), Vec3(0.5F, 2.0F, 0.0F)};
    mesh.normals = {Vec3(0.0F, 0.0F, 1.0F), Vec3(0.0F, 0.0F, 1.0F), Vec3(0.0F, 0.0F, 1.0F), Vec3(0.0F, 0.0F, 1.0F), Vec3(0.0F, 0.0F, -1.0F)};
    mesh.tangents = {Vec4(1.0F, 0.0F, 0.0F, 1.0F), Vec4(1.0F, 0.0F, 0.0F, 1.0F), Vec4(1.0F, 0.0F, 0.0F, 1.0F), Vec4(1.0F, 0.0F, 0.0F, 1.0F), Vec4(-1.0F, 0.0F, 0.0F, -1.0F)};
    mesh.uv0 = {Vec2(0.0F, 0.0F), Vec2(1.0F, 0.0F), Vec2(1.0F, 1.0F), Vec2(0.0F, 1.0F), Vec2(0.5F, 0.25F)};
    mesh.indices = {0, 1, 2, 2, 3, 0, 3, 2, 4};
    mesh.slots = {MeshSlot{"Stone", 0, 6, STONE_MATERIAL_ID}, MeshSlot{"Glass", 6, 3, GLASS_MATERIAL_ID}};
    return mesh;
}

static std::vector<std::byte> writeMesh(const Mesh& mesh) {
    std::vector<std::byte> bytes;
    mesh.write(bytes);
    return bytes;
}

static std::vector<std::byte> writeVersion1Mesh() {
    const Mesh triangle = makeTriangle();
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(TechEngine::BlobKind{TechEngine::MESH_KIND.magic, 1});
    writer.write(MESH_ASSET_ID.value());
    writer.writeSpan(std::span<const Vec3>{triangle.positions});
    writer.writeSpan(std::span<const std::uint32_t>{triangle.indices});
    return bytes;
}

static std::vector<std::byte> writeMeshWithStreamFlags(const std::uint32_t streams) {
    const Mesh triangle = makeTriangle();
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(TechEngine::MESH_KIND);
    writer.write(MESH_ASSET_ID.value());
    ImportBlock{}.write(writer);
    writer.write(streams);
    writer.writeSpan(std::span<const Vec3>{triangle.positions});
    writer.writeSpan(std::span<const std::uint32_t>{triangle.indices});
    writer.write(static_cast<std::uint32_t>(1));
    writer.write("Slot 0");
    writer.write(static_cast<std::uint32_t>(0));
    writer.write(static_cast<std::uint32_t>(3));
    writer.write(static_cast<std::uint64_t>(0));
    return bytes;
}

class MeshLoadSource final : public TechEngine::AssetSource {
private:
    std::unordered_map<ResourceId, std::vector<std::byte>> m_assets;

public:
    void addAsset(const ResourceId id, std::vector<std::byte> bytes) {
        m_assets[id] = std::move(bytes);
    }

    AssetAvailability find(const ResourceId id) const override {
        return m_assets.contains(id) ? AssetAvailability::Present : AssetAvailability::Unknown;
    }

    std::error_code read(const ResourceId id, std::vector<std::byte>& out) const override {
        out = m_assets.at(id);
        return {};
    }
};

class MeshLoadHarness {
public:
    TechEngine::JobSystem jobs;
    MeshLoadSource source;
    ResourceCache cache{jobs};

    MeshLoadHarness() {
        cache.setSource(source);
        cache.registerLoader<Mesh>(TechEngine::MESH_KIND, TechEngine::MESH_TYPE_TAG, &Mesh::loadPayload);
    }
};

TEST_CASE("a mesh with two slots and every stream writes and reads back equal", "[core][mesh]") {
    const Mesh written = makeFullMesh();
    Mesh loaded;

    REQUIRE(loaded.read(writeMesh(written)) == std::error_code{});

    CHECK(loaded.id == written.id);
    CHECK(loaded.importBlock == written.importBlock);
    CHECK(loaded.positions == written.positions);
    CHECK(loaded.normals == written.normals);
    CHECK(loaded.tangents == written.tangents);
    CHECK(loaded.uv0 == written.uv0);
    CHECK(loaded.indices == written.indices);
    CHECK(loaded.slots == written.slots);
    CHECK(loaded == written);
}

TEST_CASE("a mesh with positions only reads back with empty optional streams", "[core][mesh]") {
    const Mesh written = makeTriangle();
    Mesh loaded = makeFullMesh();

    REQUIRE(loaded.read(writeMesh(written)) == std::error_code{});

    CHECK(loaded.normals.empty());
    CHECK(loaded.tangents.empty());
    CHECK(loaded.uv0.empty());
    CHECK(loaded.importBlock == ImportBlock{});
    CHECK(loaded == written);
}

TEST_CASE("each optional stream round-trips without the others", "[core][mesh]") {
    const Mesh full = makeFullMesh();
    Mesh written = full;
    written.normals.clear();
    written.tangents.clear();
    written.uv0.clear();

    SECTION("normals alone") {
        written.normals = full.normals;
    }

    SECTION("tangents alone") {
        written.tangents = full.tangents;
    }

    SECTION("UV0 alone") {
        written.uv0 = full.uv0;
    }

    SECTION("tangents and UV0 without normals") {
        written.tangents = full.tangents;
        written.uv0 = full.uv0;
    }

    Mesh loaded;
    REQUIRE(loaded.read(writeMesh(written)) == std::error_code{});
    CHECK(loaded == written);
}

TEST_CASE("an empty mesh with one empty slot round-trips", "[core][mesh]") {
    Mesh empty;
    empty.id = MESH_ASSET_ID;
    empty.slots = {MeshSlot{"Slot 0", 0, 0, ResourceId{}}};
    Mesh loaded = makeFullMesh();

    REQUIRE(loaded.read(writeMesh(empty)) == std::error_code{});

    CHECK(loaded == empty);
}

TEST_CASE("read refuses slots that do not cover the indices in whole triangles", "[core][mesh]") {
    Mesh written = makeFullMesh();

    SECTION("a slot range past the indices") {
        written.slots[1].indexCount = 6;
    }

    SECTION("a gap between slots") {
        written.slots[0].indexCount = 3;
    }

    SECTION("an overlap between slots") {
        written.slots[1].firstIndex = 3;
        written.slots[1].indexCount = 6;
    }

    SECTION("a first slot that does not start at index 0") {
        written.slots[0].firstIndex = 3;
        written.slots[0].indexCount = 3;
    }

    SECTION("slots that end before the last index") {
        written.slots.pop_back();
    }

    SECTION("a range that is not whole triangles") {
        written.slots[0].indexCount = 5;
        written.slots[1].firstIndex = 5;
        written.slots[1].indexCount = 4;
    }

    SECTION("no slots at all") {
        written.slots.clear();
    }

    Mesh loaded;
    REQUIRE(loaded.read(writeMesh(written)) == ReadError::Malformed);
    CHECK(loaded == Mesh{});
}

TEST_CASE("read refuses a version 1 mesh with BadVersion", "[core][mesh]") {
    Mesh loaded;

    REQUIRE(loaded.read(writeVersion1Mesh()) == ReadError::BadVersion);

    CHECK(loaded == Mesh{});
}

TEST_CASE("read rejects a mesh whose bytes break the format and leaves the mesh untouched", "[core][mesh]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("an index past the last position") {
        Mesh mesh = makeTriangle();
        mesh.indices = {0, 1, 3};
        bytes = writeMesh(mesh);
        expected = ReadError::Malformed;
    }

    SECTION("an index count that is not whole triangles") {
        Mesh mesh = makeTriangle();
        mesh.indices = {0, 1};
        mesh.slots[0].indexCount = 2;
        bytes = writeMesh(mesh);
        expected = ReadError::Malformed;
    }

    SECTION("a normal stream shorter than the positions") {
        Mesh mesh = makeFullMesh();
        mesh.normals.pop_back();
        bytes = writeMesh(mesh);
        expected = ReadError::Malformed;
    }

    SECTION("a tangent stream longer than the positions") {
        Mesh mesh = makeFullMesh();
        mesh.tangents.push_back(Vec4(1.0F, 0.0F, 0.0F, 1.0F));
        bytes = writeMesh(mesh);
        expected = ReadError::Malformed;
    }

    SECTION("a UV0 stream shorter than the positions") {
        Mesh mesh = makeFullMesh();
        mesh.uv0.resize(1);
        bytes = writeMesh(mesh);
        expected = ReadError::Malformed;
    }

    SECTION("a stream flag no reader knows") {
        bytes = writeMeshWithStreamFlags(1u << 3);
        expected = ReadError::Malformed;
    }

    SECTION("a normal flag whose stream holds no values") {
        const Mesh mesh = makeTriangle();
        std::vector<std::byte> flagged;
        Writer writer{flagged};
        writer.writeHeader(TechEngine::MESH_KIND);
        writer.write(MESH_ASSET_ID.value());
        ImportBlock{}.write(writer);
        writer.write(static_cast<std::uint32_t>(1));
        writer.writeSpan(std::span<const Vec3>{mesh.positions});
        writer.writeSpan(std::span<const Vec3>{});
        writer.writeSpan(std::span<const std::uint32_t>{mesh.indices});
        writer.write(static_cast<std::uint32_t>(1));
        writer.write("Slot 0");
        writer.write(static_cast<std::uint32_t>(0));
        writer.write(static_cast<std::uint32_t>(3));
        writer.write(static_cast<std::uint64_t>(0));
        bytes = std::move(flagged);
        expected = ReadError::Malformed;
    }

    SECTION("bytes after the slots") {
        bytes = writeMesh(makeFullMesh());
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the slots") {
        bytes = writeMesh(makeFullMesh());
        bytes.resize(bytes.size() - 1);
        expected = ReadError::Truncated;
    }

    SECTION("a slot count larger than the bytes left") {
        bytes = writeMesh(makeTriangle());
        const std::size_t slotSize = sizeof(std::uint32_t) + 6 + 2 * sizeof(std::uint32_t) + sizeof(std::uint64_t);
        const std::size_t slotCountOffset = bytes.size() - slotSize - sizeof(std::uint32_t);
        bytes[slotCountOffset] = std::byte{0xFF};
        bytes[slotCountOffset + 1] = std::byte{0xFF};
        expected = ReadError::Truncated;
    }

    SECTION("another asset kind's magic") {
        std::vector<std::byte> scene;
        Writer writer{scene};
        writer.writeHeader(TechEngine::SCENE_KIND);
        writer.write(MESH_ASSET_ID.value());
        bytes = std::move(scene);
        expected = ReadError::BadMagic;
    }

    SECTION("a newer mesh version") {
        std::vector<std::byte> newer;
        Writer writer{newer};
        writer.writeHeader(TechEngine::BlobKind{TechEngine::MESH_KIND.magic, static_cast<std::uint16_t>(TechEngine::MESH_KIND.version + 1)});
        writer.write(MESH_ASSET_ID.value());
        bytes = std::move(newer);
        expected = ReadError::BadVersion;
    }

    Mesh loaded;
    REQUIRE(loaded.read(bytes) == expected);
    CHECK(loaded == Mesh{});
}

TEST_CASE("the mesh loader lists every non-null default material and no null one", "[core][mesh][resources]") {
    Mesh written = makeFullMesh();
    written.slots = {MeshSlot{"Stone", 0, 3, STONE_MATERIAL_ID}, MeshSlot{"Bare", 3, 3, ResourceId{}}, MeshSlot{"Glass", 6, 3, GLASS_MATERIAL_ID}};
    Mesh payload;
    std::vector<ResourceId> dependencies{ResourceId{0x6D65736800000003}};

    REQUIRE(Mesh::loadPayload(writeMesh(written), &payload, dependencies) == std::error_code{});

    CHECK(payload == written);
    CHECK(dependencies == std::vector<ResourceId>{STONE_MATERIAL_ID, GLASS_MATERIAL_ID});
}

TEST_CASE("the mesh loader names no dependencies when every default is null", "[core][mesh][resources]") {
    Mesh payload;
    std::vector<ResourceId> dependencies{ResourceId{0x6D65736800000003}};

    REQUIRE(Mesh::loadPayload(writeMesh(makeTriangle()), &payload, dependencies) == std::error_code{});

    CHECK(payload == makeTriangle());
    CHECK(dependencies.empty());
}

TEST_CASE("the mesh loader names no dependencies when its bytes are refused", "[core][mesh][resources]") {
    Mesh written = makeFullMesh();
    written.slots.pop_back();
    Mesh payload;
    std::vector<ResourceId> dependencies{ResourceId{0x6D65736800000003}};

    REQUIRE(Mesh::loadPayload(writeMesh(written), &payload, dependencies) == ReadError::Malformed);

    CHECK(dependencies.empty());
}

TEST_CASE("a cache with the mesh loader registered loads a written mesh back", "[core][mesh][resources]") {
    MeshLoadHarness harness;
    harness.source.addAsset(MESH_ASSET_ID, writeMesh(makeTriangle()));
    ResourceHandle<Mesh> handle;

    REQUIRE(harness.cache.request(MESH_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    REQUIRE(state == ResourceState::Ready);
    const Mesh* mesh = harness.cache.resolve(handle);
    REQUIRE(mesh != nullptr);
    CHECK(*mesh == makeTriangle());

    harness.cache.release(handle);
}

TEST_CASE("a cache fails the entry of a mesh whose bytes are malformed", "[core][mesh][resources]") {
    MeshLoadHarness harness;
    Mesh malformed = makeTriangle();
    malformed.indices = {0, 0, 3};
    harness.source.addAsset(MESH_ASSET_ID, writeMesh(malformed));
    ResourceHandle<Mesh> handle;

    REQUIRE(harness.cache.request(MESH_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Malformed);

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    CHECK(state == ResourceState::Failed);
    CHECK(harness.cache.resolve(handle) == nullptr);

    harness.cache.release(handle);
}

TEST_CASE("a cache fails the entry of a version 1 mesh", "[core][mesh][resources]") {
    MeshLoadHarness harness;
    harness.source.addAsset(MESH_ASSET_ID, writeVersion1Mesh());
    ResourceHandle<Mesh> handle;

    REQUIRE(harness.cache.request(MESH_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::BadVersion);

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    CHECK(state == ResourceState::Failed);

    harness.cache.release(handle);
}
