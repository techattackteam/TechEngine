#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>
#include <TechEngine/core/serialization/Visit.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

using TechEngine::AssetRef;
using TechEngine::Entity;
using TechEngine::ReferenceField;
using TechEngine::ReferenceVisitor;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;

struct ReferenceVisitorAsset {};

struct ReferenceVisitorNested {
    std::uint32_t count = 0;
    Entity owner;

    template<typename Archive>
    friend void visit(Archive& archive, ReferenceVisitorNested& value) {
        archive.field("count", value.count);
        archive.field("owner", value.owner);
    }
};

struct ReferenceVisitorSample {
    std::uint32_t value = 0;
    Entity first;
    float weight = 0.0f;
    AssetRef<ReferenceVisitorAsset> asset;
    std::vector<std::uint32_t> indices;
    ReferenceVisitorNested nested;
    Entity second;

    template<typename Archive>
    friend void visit(Archive& archive, ReferenceVisitorSample& value) {
        archive.field("value", value.value);
        archive.field("first", value.first);
        archive.field("weight", value.weight);
        archive.field("asset", value.asset);
        archive.field("indices", value.indices);
        archive.field("nested", value.nested);
        archive.field("second", value.second);
    }
};

class RecordingReferenceVisitor final : public ReferenceVisitor {
public:
    std::vector<Entity> entities;
    std::vector<ResourceId> assets;

    void visitEntity(Entity& value) override {
        entities.push_back(value);
    }

    void visitAsset(const ResourceId id, ResourceSlotId&) override {
        assets.push_back(id);
    }
};

class RewritingReferenceVisitor final : public ReferenceVisitor {
public:
    Entity entityReplacement;
    ResourceSlotId handleReplacement;

    void visitEntity(Entity& value) override {
        value = entityReplacement;
    }

    void visitAsset(ResourceId, ResourceSlotId& handle) override {
        handle = handleReplacement;
    }
};

static constexpr ResourceId REFERENCE_VISITOR_ASSET_ID{0xA55E70000000C0DE};

static constexpr ResourceId SECOND_REFERENCE_VISITOR_ASSET_ID{0xA55E70000000C0DF};

struct ReferenceVisitorElement {
    AssetRef<ReferenceVisitorAsset> asset;
    Entity owner;

    template<typename Archive>
    friend void visit(Archive& archive, ReferenceVisitorElement& value) {
        archive.field("asset", value.asset);
        archive.field("owner", value.owner);
    }
};

struct ReferenceVisitorList {
    Entity before;
    std::vector<ReferenceVisitorElement> elements;
    Entity after;

    template<typename Archive>
    friend void visit(Archive& archive, ReferenceVisitorList& value) {
        archive.field("before", value.before);
        archive.field("elements", value.elements);
        archive.field("after", value.after);
    }
};

static ReferenceVisitorList makeFullList() {
    ReferenceVisitorList list;
    list.before = Entity{1, 0};
    list.elements = {
        ReferenceVisitorElement{AssetRef<ReferenceVisitorAsset>{REFERENCE_VISITOR_ASSET_ID, ResourceHandle<ReferenceVisitorAsset>{}}, Entity{2, 0}},
        ReferenceVisitorElement{AssetRef<ReferenceVisitorAsset>{SECOND_REFERENCE_VISITOR_ASSET_ID, ResourceHandle<ReferenceVisitorAsset>{}}, Entity{3, 0}},
    };
    list.after = Entity{4, 0};
    return list;
}

static ReferenceVisitorSample makeFullSample() {
    ReferenceVisitorSample sample;
    sample.value = 7;
    sample.first = Entity{3, 1};
    sample.weight = 2.5f;
    sample.asset = AssetRef<ReferenceVisitorAsset>{REFERENCE_VISITOR_ASSET_ID, ResourceHandle<ReferenceVisitorAsset>{}};
    sample.indices = {1, 2, 3};
    sample.nested = ReferenceVisitorNested{4, Entity{9, 2}};
    sample.second = Entity{5, 0};
    return sample;
}

TEST_CASE("Entity and AssetRef are the only reference fields", "[core][serialization]") {
    STATIC_REQUIRE(ReferenceField<Entity>);
    STATIC_REQUIRE(ReferenceField<AssetRef<ReferenceVisitorAsset>>);
    STATIC_REQUIRE_FALSE(ReferenceField<ResourceId>);
    STATIC_REQUIRE_FALSE(ReferenceField<ResourceHandle<ReferenceVisitorAsset>>);
    STATIC_REQUIRE_FALSE(ReferenceField<std::uint32_t>);
    STATIC_REQUIRE_FALSE(ReferenceField<ReferenceVisitorNested>);
}

TEST_CASE("a type with a templated visit can be walked by the reference visitor", "[core][serialization]") {
    STATIC_REQUIRE(TechEngine::Visitable<ReferenceVisitorSample, ReferenceVisitor>);
    STATIC_REQUIRE(TechEngine::Visitable<ReferenceVisitorNested, ReferenceVisitor>);
}

TEST_CASE("the reference visitor sees every Entity and AssetRef field in visit order and nothing else", "[core][serialization]") {
    ReferenceVisitorSample sample = makeFullSample();
    RecordingReferenceVisitor visitor;

    visitor.field(sample);

    const std::vector<Entity> expectedEntities{Entity{3, 1}, Entity{9, 2}, Entity{5, 0}};
    REQUIRE(visitor.entities == expectedEntities);
    const std::vector<ResourceId> expectedAssets{REFERENCE_VISITOR_ASSET_ID};
    REQUIRE(visitor.assets == expectedAssets);
    CHECK(sample.value == 7);
    CHECK(sample.weight == 2.5f);
    CHECK(sample.indices == std::vector<std::uint32_t>{1, 2, 3});
    CHECK(sample.nested.count == 4);
}

TEST_CASE("the reference visitor skips empty fields", "[core][serialization]") {
    ReferenceVisitorSample sample;
    sample.second = Entity{5, 0};
    sample.asset.handle = ResourceHandle<ReferenceVisitorAsset>{2, 1};
    RecordingReferenceVisitor visitor;

    visitor.field(sample);

    const std::vector<Entity> expectedEntities{Entity{5, 0}};
    REQUIRE(visitor.entities == expectedEntities);
    REQUIRE(visitor.assets.empty());
}

TEST_CASE("an Entity at index zero is a reference, not an empty field", "[core][serialization]") {
    ReferenceVisitorSample sample;
    sample.first = Entity{0, 0};
    RecordingReferenceVisitor visitor;

    visitor.field(sample);

    const std::vector<Entity> expectedEntities{Entity{0, 0}};
    REQUIRE(visitor.entities == expectedEntities);
}

TEST_CASE("a reference hook rewrites the field it was handed", "[core][serialization]") {
    ReferenceVisitorSample sample = makeFullSample();
    RewritingReferenceVisitor visitor;
    visitor.entityReplacement = Entity{11, 4};
    visitor.handleReplacement = ResourceSlotId{6, 2};

    visitor.field(sample);

    CHECK(sample.first == Entity{11, 4});
    CHECK(sample.nested.owner == Entity{11, 4});
    CHECK(sample.second == Entity{11, 4});
    CHECK(sample.asset.handle == ResourceHandle<ReferenceVisitorAsset>{6, 2});
    CHECK(sample.asset.id == REFERENCE_VISITOR_ASSET_ID);
}

TEST_CASE("a reference hook never sees an empty field, so an empty field stays empty", "[core][serialization]") {
    ReferenceVisitorSample sample;
    RewritingReferenceVisitor visitor;
    visitor.entityReplacement = Entity{11, 4};
    visitor.handleReplacement = ResourceSlotId{6, 2};

    visitor.field(sample);

    CHECK(sample.first == Entity{});
    CHECK(sample.second == Entity{});
    CHECK(sample.nested.owner == Entity{});
    CHECK(sample.asset == AssetRef<ReferenceVisitorAsset>{});
}

TEST_CASE("the reference visitor walks a vector of a visited type element by element, between its neighbours", "[core][serialization]") {
    ReferenceVisitorList list = makeFullList();
    RecordingReferenceVisitor visitor;

    visitor.field(list);

    const std::vector<Entity> expectedEntities{Entity{1, 0}, Entity{2, 0}, Entity{3, 0}, Entity{4, 0}};
    CHECK(visitor.entities == expectedEntities);
    const std::vector<ResourceId> expectedAssets{REFERENCE_VISITOR_ASSET_ID, SECOND_REFERENCE_VISITOR_ASSET_ID};
    CHECK(visitor.assets == expectedAssets);
}

TEST_CASE("a reference hook rewrites every element of a walked vector in place", "[core][serialization]") {
    ReferenceVisitorList list = makeFullList();
    RewritingReferenceVisitor visitor;
    visitor.entityReplacement = Entity{11, 4};
    visitor.handleReplacement = ResourceSlotId{6, 2};

    visitor.field(list);

    REQUIRE(list.elements.size() == 2);
    for (const ReferenceVisitorElement& element: list.elements) {
        CHECK(element.owner == Entity{11, 4});
        CHECK(element.asset.handle == ResourceHandle<ReferenceVisitorAsset>{6, 2});
    }
    CHECK(list.elements[0].asset.id == REFERENCE_VISITOR_ASSET_ID);
    CHECK(list.elements[1].asset.id == SECOND_REFERENCE_VISITOR_ASSET_ID);
    CHECK(list.after == Entity{11, 4});
}

TEST_CASE("an empty walked vector shows the reference visitor nothing, and the fields after it are still seen", "[core][serialization]") {
    ReferenceVisitorList list = makeFullList();
    list.elements.clear();
    RecordingReferenceVisitor visitor;

    visitor.field(list);

    const std::vector<Entity> expectedEntities{Entity{1, 0}, Entity{4, 0}};
    CHECK(visitor.entities == expectedEntities);
    CHECK(visitor.assets.empty());
}
