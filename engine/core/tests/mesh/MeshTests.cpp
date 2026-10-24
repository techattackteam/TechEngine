#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
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
using TechEngine::Mesh;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::Vec3;
using TechEngine::Writer;

static constexpr ResourceId MESH_ASSET_ID{0x6D65736800000001};

static Mesh makeTriangle(const ResourceId id = MESH_ASSET_ID) {
    Mesh mesh;
    mesh.id = id;
    mesh.positions = {Vec3(-0.5F, -0.5F, 0.0F), Vec3(0.5F, -0.5F, 0.0F), Vec3(0.0F, 0.5F, 0.0F)};
    mesh.indices = {0, 1, 2};
    return mesh;
}

static std::vector<std::byte> writeMesh(const Mesh& mesh) {
    std::vector<std::byte> bytes;
    mesh.write(bytes);
    return bytes;
}

static std::vector<std::byte> writeRawMesh(const std::vector<Vec3>& positions, const std::vector<std::uint32_t>& indices) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(TechEngine::MESH_KIND);
    writer.write(MESH_ASSET_ID.value());
    writer.writeSpan(std::span<const Vec3>{positions});
    writer.writeSpan(std::span<const std::uint32_t>{indices});
    return bytes;
}

static void checkTriangle(const Mesh& mesh) {
    const Mesh expected = makeTriangle();
    CHECK(mesh.id == expected.id);
    CHECK(mesh.positions == expected.positions);
    CHECK(mesh.indices == expected.indices);
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

TEST_CASE("a mesh written and read back keeps its id, positions and indices", "[core][mesh]") {
    Mesh loaded;

    REQUIRE(loaded.read(writeMesh(makeTriangle())) == std::error_code{});

    checkTriangle(loaded);
}

TEST_CASE("a mesh with no positions and no indices round-trips", "[core][mesh]") {
    Mesh empty;
    empty.id = MESH_ASSET_ID;
    Mesh loaded = makeTriangle(ResourceId{0x6D65736800000002});

    REQUIRE(loaded.read(writeMesh(empty)) == std::error_code{});

    CHECK(loaded.id == MESH_ASSET_ID);
    CHECK(loaded.positions.empty());
    CHECK(loaded.indices.empty());
}

TEST_CASE("read rejects a mesh whose bytes break the format and leaves the mesh untouched", "[core][mesh]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("an index past the last position") {
        bytes = writeRawMesh({Vec3(0.0F), Vec3(1.0F), Vec3(2.0F)}, {0, 1, 3});
        expected = ReadError::Malformed;
    }

    SECTION("an index count that is not whole triangles") {
        bytes = writeRawMesh({Vec3(0.0F), Vec3(1.0F), Vec3(2.0F)}, {0, 1});
        expected = ReadError::Malformed;
    }

    SECTION("bytes after the indices") {
        bytes = writeMesh(makeTriangle());
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the indices") {
        bytes = writeMesh(makeTriangle());
        bytes.resize(bytes.size() - 1);
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
    CHECK(loaded.id == ResourceId{});
    CHECK(loaded.positions.empty());
    CHECK(loaded.indices.empty());
}

TEST_CASE("the mesh loader decodes its bytes into the payload and names no dependencies", "[core][mesh][resources]") {
    Mesh payload;
    std::vector<ResourceId> dependencies{ResourceId{0x6D65736800000003}};

    REQUIRE(Mesh::loadPayload(writeMesh(makeTriangle()), &payload, dependencies) == std::error_code{});

    checkTriangle(payload);
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
    checkTriangle(*mesh);

    harness.cache.release(handle);
}

TEST_CASE("a cache fails the entry of a mesh whose bytes are malformed", "[core][mesh][resources]") {
    MeshLoadHarness harness;
    harness.source.addAsset(MESH_ASSET_ID, writeRawMesh({Vec3(0.0F)}, {0, 0, 1}));
    ResourceHandle<Mesh> handle;

    REQUIRE(harness.cache.request(MESH_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Malformed);

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    CHECK(state == ResourceState::Failed);
    CHECK(harness.cache.resolve(handle) == nullptr);

    harness.cache.release(handle);
}
