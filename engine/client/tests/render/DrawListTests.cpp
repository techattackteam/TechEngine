#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/material/Material.hpp>

#include <render/DrawList.hpp>
#include <render/device/draw/IndexedIndirectCommand.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

using TechEngine::DrawBatch;
using TechEngine::DrawBucket;
using TechEngine::DrawInstance;
using TechEngine::DrawItem;
using TechEngine::DrawList;
using TechEngine::IndexedIndirectCommand;
using TechEngine::Mat4;
using TechEngine::MaterialAlphaMode;
using TechEngine::Vec3;

static DrawItem itemOf(const std::uint32_t instance, const std::uint32_t materialRecord, const std::uint32_t firstIndex = 0, const DrawBucket bucket = DrawBucket::Opaque, const std::uint32_t block = 0) {
    DrawItem item;
    item.block = block;
    item.bucket = bucket;
    item.firstIndex = firstIndex;
    item.indexCount = 36;
    item.baseVertex = 100;
    item.materialRecord = materialRecord;
    item.instance = instance;
    return item;
}

TEST_CASE("instances that share a block, bucket, index range and material draw as one command, one draw instance each", "[client][render][draw]") {
    DrawList list;
    list.add(itemOf(0, 5));
    list.add(itemOf(3, 5));
    list.add(itemOf(7, 5));

    list.build();

    REQUIRE(list.getCommands() == std::vector<IndexedIndirectCommand>{IndexedIndirectCommand{36, 3, 0, 100, 0}});
    REQUIRE(list.getBatches() == std::vector<DrawBatch>{DrawBatch{0, DrawBucket::Opaque, 0, 1}});
    std::vector<std::uint32_t> worlds;
    for (const DrawInstance& instance: list.getInstances()) {
        CHECK(instance.materialRecord == 5);
        worlds.push_back(instance.instance);
    }
    std::ranges::sort(worlds);
    CHECK(worlds == std::vector<std::uint32_t>{0, 3, 7});
}

TEST_CASE("a different material, index range or block splits a command, and a different bucket or block splits the batch", "[client][render][draw]") {
    DrawList list;
    list.add(itemOf(0, 5));
    list.add(itemOf(1, 6));
    list.add(itemOf(2, 5, 36));
    list.add(itemOf(3, 5, 0, DrawBucket::Opaque, 1));
    list.add(itemOf(4, 5, 0, DrawBucket::Cutout));

    list.build();

    REQUIRE(list.getCommands().size() == 5);
    REQUIRE(list.getInstances().size() == 5);
    for (const IndexedIndirectCommand& command: list.getCommands()) {
        CHECK(command.instanceCount == 1);
    }
    REQUIRE(
        list.getBatches() == std::vector<DrawBatch>{
                                 DrawBatch{0, DrawBucket::Opaque, 0, 3},
                                 DrawBatch{1, DrawBucket::Opaque, 3, 1},
                                 DrawBatch{0, DrawBucket::Cutout, 4, 1},
                             });
}

TEST_CASE("each command's baseInstance is its first draw instance, and its instances are contiguous", "[client][render][draw]") {
    DrawList list;
    list.add(itemOf(0, 1));
    list.add(itemOf(1, 2));
    list.add(itemOf(2, 1));
    list.add(itemOf(3, 2));
    list.add(itemOf(4, 2));

    list.build();

    const std::vector<IndexedIndirectCommand>& commands = list.getCommands();
    const std::vector<DrawInstance>& instances = list.getInstances();
    REQUIRE(commands.size() == 2);
    std::uint32_t expectedBase = 0;
    std::uint32_t totalInstances = 0;
    for (const IndexedIndirectCommand& command: commands) {
        CHECK(command.baseInstance == expectedBase);
        REQUIRE(command.baseInstance + command.instanceCount <= instances.size());
        const std::uint32_t material = instances[command.baseInstance].materialRecord;
        for (std::uint32_t i = 0; i < command.instanceCount; i++) {
            CHECK(instances[command.baseInstance + i].materialRecord == material);
        }
        expectedBase += command.instanceCount;
        totalInstances += command.instanceCount;
    }
    CHECK(totalInstances == 5);
}

TEST_CASE("batches run in bucket order before block order, so every blended draw comes last", "[client][render][draw]") {
    DrawList list;
    list.add(itemOf(0, 1, 0, DrawBucket::Blended, 0));
    list.add(itemOf(1, 1, 0, DrawBucket::Opaque, 1));
    list.add(itemOf(2, 1, 0, DrawBucket::CutoutDoubleSided, 0));
    list.add(itemOf(3, 1, 0, DrawBucket::OpaqueMirrored, 0));
    list.add(itemOf(4, 1, 0, DrawBucket::Opaque, 0));

    list.build();

    std::vector<DrawBucket> buckets;
    std::vector<std::uint32_t> blocks;
    for (const DrawBatch& batch: list.getBatches()) {
        buckets.push_back(batch.bucket);
        blocks.push_back(batch.block);
    }
    CHECK(buckets == std::vector<DrawBucket>{DrawBucket::Opaque, DrawBucket::Opaque, DrawBucket::OpaqueMirrored, DrawBucket::CutoutDoubleSided, DrawBucket::Blended});
    CHECK(blocks == std::vector<std::uint32_t>{0, 1, 0, 0, 0});
}

TEST_CASE("a cleared list keeps nothing from the frame before, and an empty build has no batch", "[client][render][draw]") {
    DrawList list;
    list.add(itemOf(0, 1));
    list.build();
    REQUIRE_FALSE(list.getBatches().empty());

    list.clear();
    list.build();

    CHECK(list.getCommands().empty());
    CHECK(list.getInstances().empty());
    CHECK(list.getBatches().empty());
}

TEST_CASE("a single-sided material on a mirrored world takes its bucket's twin, and a double-sided one ignores the mirror", "[client][render][draw]") {
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Opaque, false, false) == DrawBucket::Opaque);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Opaque, false, true) == DrawBucket::OpaqueMirrored);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Opaque, true, false) == DrawBucket::OpaqueDoubleSided);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Opaque, true, true) == DrawBucket::OpaqueDoubleSided);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Cutout, false, false) == DrawBucket::Cutout);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Cutout, false, true) == DrawBucket::CutoutMirrored);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Cutout, true, true) == DrawBucket::CutoutDoubleSided);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Blended, false, false) == DrawBucket::Blended);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Blended, false, true) == DrawBucket::BlendedMirrored);
    CHECK(DrawList::selectBucket(MaterialAlphaMode::Blended, true, true) == DrawBucket::BlendedDoubleSided);
}

TEST_CASE("a world with one or three negative scales is mirrored, and a rotation, a translation or two negative scales are not", "[client][render][draw]") {
    const Mat4 identity = Mat4(1.0f);
    CHECK_FALSE(DrawList::isMirrored(identity));
    CHECK_FALSE(DrawList::isMirrored(glm::translate(identity, Vec3{4.0f, -2.0f, 9.0f})));
    CHECK_FALSE(DrawList::isMirrored(glm::rotate(identity, glm::radians(135.0f), Vec3{0.0f, 1.0f, 0.0f})));
    CHECK_FALSE(DrawList::isMirrored(glm::scale(identity, Vec3{-1.0f, -1.0f, 1.0f})));
    CHECK(DrawList::isMirrored(glm::scale(identity, Vec3{-1.0f, 1.0f, 1.0f})));
    CHECK(DrawList::isMirrored(glm::scale(identity, Vec3{-2.0f, -0.5f, -3.0f})));
    CHECK(DrawList::isMirrored(glm::rotate(glm::scale(identity, Vec3{1.0f, 1.0f, -1.0f}), glm::radians(30.0f), Vec3{1.0f, 0.0f, 0.0f})));
}
