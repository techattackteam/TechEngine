#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <render/GeometryBuffers.hpp>
#include <render/device/RenderDevice.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

using TechEngine::GeometryAttributes;
using TechEngine::GeometryBuffers;
using TechEngine::GeometryCapacity;
using TechEngine::GeometryRange;
using TechEngine::Mesh;
using TechEngine::Vec2;
using TechEngine::Vec3;
using TechEngine::Vec4;

static constexpr GeometryCapacity SMALL_BLOCK{100, 300};

static bool spansOverlap(const std::uint32_t firstStart, const std::uint32_t firstCount, const std::uint32_t secondStart, const std::uint32_t secondCount) {
    return firstStart < secondStart + secondCount && secondStart < firstStart + firstCount;
}

static bool rangesOverlap(const GeometryRange& first, const GeometryRange& second) {
    if (first.block != second.block) {
        return false;
    }
    return spansOverlap(first.baseVertex, first.vertexCount, second.baseVertex, second.vertexCount) || spansOverlap(first.firstIndex, first.indexCount, second.firstIndex, second.indexCount);
}

static bool fitsItsBlock(const GeometryBuffers& geometry, const GeometryRange& range) {
    const GeometryCapacity capacity = geometry.getBlock(range.block).capacity;
    return range.baseVertex + range.vertexCount <= capacity.vertices && range.firstIndex + range.indexCount <= capacity.indices;
}

TEST_CASE("a freed range is reused by the next mesh that needs the same size", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange first = geometry.allocate(10, 30);
    const GeometryRange second = geometry.allocate(10, 30);

    geometry.free(first);
    const GeometryRange reused = geometry.allocate(10, 30);

    REQUIRE(reused == first);
    REQUIRE_FALSE(rangesOverlap(reused, second));
    REQUIRE(geometry.getBlockCount() == 1);
}

TEST_CASE("a freed range in a full block is reused before another block is added", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange first = geometry.allocate(60, 180);
    const GeometryRange rest = geometry.allocate(40, 120);
    REQUIRE(first.block == rest.block);

    geometry.free(first);
    const GeometryRange reused = geometry.allocate(60, 180);

    REQUIRE(reused.block == first.block);
    REQUIRE(geometry.getBlockCount() == 1);
    REQUIRE_FALSE(rangesOverlap(reused, rest));
}

TEST_CASE("freed neighbours merge on either side and on both, until the block is one free range again", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    std::vector<GeometryRange> ranges;
    for (int i = 0; i < 5; i++) {
        ranges.push_back(geometry.allocate(20, 60));
    }
    REQUIRE(geometry.getBlockCount() == 1);

    geometry.free(ranges[1]);
    geometry.free(ranges[3]);
    geometry.free(ranges[2]);
    geometry.free(ranges[0]);
    geometry.free(ranges[4]);
    const GeometryRange whole = geometry.allocate(100, 300);

    REQUIRE(whole.block == ranges[0].block);
    REQUIRE(whole.baseVertex == 0);
    REQUIRE(whole.firstIndex == 0);
    REQUIRE(geometry.getBlockCount() == 1);
}

TEST_CASE("freeing a range twice fires a check and frees it only once", "[client][render][geometry]") {
    const TechEngineTests::AssertHandlerGuard guard;
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange first = geometry.allocate(50, 150);
    const GeometryRange second = geometry.allocate(50, 150);
    REQUIRE(geometry.getBlockCount() == 1);

    geometry.free(first);
    REQUIRE(TechEngineTests::g_fired.empty());
    geometry.free(first);
    REQUIRE(TechEngineTests::g_fired.size() == 2);

    const GeometryRange reused = geometry.allocate(50, 150);
    const GeometryRange next = geometry.allocate(50, 150);
    REQUIRE(reused == first);
    REQUIRE_FALSE(rangesOverlap(reused, second));
    REQUIRE(next.block != first.block);
    REQUIRE(geometry.getBlockCount() == 2);
}

TEST_CASE("freeing an empty range does nothing, even with no blocks", "[client][render][geometry]") {
    const TechEngineTests::AssertHandlerGuard guard;
    GeometryBuffers geometry{SMALL_BLOCK};

    geometry.free(GeometryRange{});

    REQUIRE(TechEngineTests::g_fired.empty());
    REQUIRE(geometry.getBlockCount() == 0);
}

TEST_CASE("a mesh never moves: no live range is handed out again, through frees and new blocks", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    std::vector<GeometryRange> live;
    for (int i = 0; i < 40; i++) {
        live.push_back(geometry.allocate(7, 18));
    }
    const std::vector<GeometryRange> before = live;
    std::vector<GeometryRange> kept;
    for (std::size_t i = 0; i < live.size(); i++) {
        if (i % 2 == 0) {
            geometry.free(live[i]);
        } else {
            kept.push_back(live[i]);
        }
    }
    for (int i = 0; i < 30; i++) {
        kept.push_back(geometry.allocate(5, 12));
    }

    REQUIRE(geometry.getBlockCount() >= 3);
    for (std::size_t i = 1; i < before.size(); i += 2) {
        REQUIRE(kept[i / 2] == before[i]);
    }
    for (std::size_t i = 0; i < kept.size(); i++) {
        REQUIRE(fitsItsBlock(geometry, kept[i]));
        for (std::size_t j = i + 1; j < kept.size(); j++) {
            REQUIRE_FALSE(rangesOverlap(kept[i], kept[j]));
        }
    }
}

TEST_CASE("a full block adds another, and the new mesh goes in it", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange first = geometry.allocate(60, 180);

    const GeometryRange second = geometry.allocate(60, 180);

    REQUIRE(geometry.getBlockCount() == 2);
    REQUIRE(second.block != first.block);
    REQUIRE(geometry.getBlock(second.block).capacity.vertices == SMALL_BLOCK.vertices);
    REQUIRE(geometry.getBlock(second.block).capacity.indices == SMALL_BLOCK.indices);
}

TEST_CASE("a block whose indices are full adds another, even with vertices to spare", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange first = geometry.allocate(10, 290);

    const GeometryRange second = geometry.allocate(10, 30);

    REQUIRE(geometry.getBlockCount() == 2);
    REQUIRE(second.block != first.block);
}

TEST_CASE("a mesh larger than an empty block gets a block sized exactly to it", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange normal = geometry.allocate(10, 30);

    const GeometryRange oversized = geometry.allocate(150, 30);
    const GeometryRange after = geometry.allocate(10, 30);

    REQUIRE(oversized.block != normal.block);
    REQUIRE(geometry.getBlock(oversized.block).capacity.vertices == 150);
    REQUIRE(geometry.getBlock(oversized.block).capacity.indices == 30);
    REQUIRE(after.block == normal.block);
    REQUIRE(geometry.getBlockCount() == 2);
}

TEST_CASE("a mesh with more indices than an empty block holds gets its own block too", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};

    const GeometryRange oversized = geometry.allocate(10, 400);

    REQUIRE(geometry.getBlockCount() == 1);
    REQUIRE(geometry.getBlock(oversized.block).capacity.vertices == 10);
    REQUIRE(geometry.getBlock(oversized.block).capacity.indices == 400);
}

TEST_CASE("a stream the mesh lacks is filled with import's defaults", "[client][render][geometry]") {
    Mesh mesh;
    mesh.positions = {Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}};
    mesh.normals = {Vec3{0.0F, 0.0F, 1.0F}, Vec3{1.0F, 0.0F, 0.0F}};
    std::vector<GeometryAttributes> attributes(5);

    GeometryBuffers::interleaveAttributes(mesh, attributes);

    REQUIRE(attributes.size() == 2);
    REQUIRE(attributes[0].normal == Vec3{0.0F, 0.0F, 1.0F});
    REQUIRE(attributes[1].normal == Vec3{1.0F, 0.0F, 0.0F});
    for (const GeometryAttributes& vertex: attributes) {
        REQUIRE(vertex.tangent == Vec4{1.0F, 0.0F, 0.0F, 1.0F});
        REQUIRE(vertex.uv0 == Vec2{0.0F, 0.0F});
    }
}

TEST_CASE("a mesh with every stream keeps each value at its vertex", "[client][render][geometry]") {
    Mesh mesh;
    mesh.positions = {Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}};
    mesh.normals = {Vec3{0.0F, 1.0F, 0.0F}, Vec3{0.0F, 0.0F, 1.0F}};
    mesh.tangents = {Vec4{0.0F, 0.0F, 1.0F, -1.0F}, Vec4{1.0F, 0.0F, 0.0F, 1.0F}};
    mesh.uv0 = {Vec2{0.25F, 0.5F}, Vec2{0.75F, 1.0F}};
    std::vector<GeometryAttributes> attributes;

    GeometryBuffers::interleaveAttributes(mesh, attributes);

    REQUIRE(attributes.size() == 2);
    for (std::size_t i = 0; i < attributes.size(); i++) {
        REQUIRE(attributes[i].normal == mesh.normals[i]);
        REQUIRE(attributes[i].tangent == mesh.tangents[i]);
        REQUIRE(attributes[i].uv0 == mesh.uv0[i]);
    }
}

TEST_CASE("a mesh with no vertices is placed with no range and no device", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    GeometryRange range;
    range.vertexCount = 99;

    const std::error_code error = geometry.place(Mesh{}, range);

    REQUIRE(error == std::error_code{});
    REQUIRE(range == GeometryRange{});
    REQUIRE(geometry.getBlockCount() == 0);
}

TEST_CASE("a mesh with vertices but no indices is placed with no range and no device", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};
    Mesh mesh;
    mesh.positions = {Vec3{0.0F, 0.0F, 0.0F}, Vec3{1.0F, 0.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F}};
    GeometryRange range;
    range.vertexCount = 99;

    const std::error_code error = geometry.place(mesh, range);

    REQUIRE(error == std::error_code{});
    REQUIRE(range == GeometryRange{});
    REQUIRE(geometry.getBlockCount() == 0);
}

TEST_CASE("allocating zero vertices or zero indices returns an empty range and adds no block", "[client][render][geometry]") {
    GeometryBuffers geometry{SMALL_BLOCK};

    const GeometryRange noVertices = geometry.allocate(0, 30);
    const GeometryRange noIndices = geometry.allocate(10, 0);

    REQUIRE(noVertices == GeometryRange{});
    REQUIRE(noIndices == GeometryRange{});
    REQUIRE(geometry.getBlockCount() == 0);
}

TEST_CASE("freeing a range in a block that does not exist fires a check and changes nothing", "[client][render][geometry]") {
    const TechEngineTests::AssertHandlerGuard guard;
    GeometryBuffers geometry{SMALL_BLOCK};
    const GeometryRange live = geometry.allocate(10, 30);
    GeometryRange stray = live;
    stray.block = 5;

    geometry.free(stray);
    const GeometryRange next = geometry.allocate(10, 30);

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(geometry.getBlockCount() == 1);
    REQUIRE_FALSE(rangesOverlap(next, live));
}

TEST_CASE("a first block the device refuses fails initialize with the device's error and creates nothing", "[client][render][geometry]") {
    TechEngine::RenderDevice device;
    GeometryBuffers geometry{GeometryCapacity{0, 0}};

    const std::error_code error = geometry.initialize(device);

    REQUIRE(error == TechEngine::ClientError::InvalidRenderDescription);
    REQUIRE(device.getLiveObjectCount() == 0);
}
