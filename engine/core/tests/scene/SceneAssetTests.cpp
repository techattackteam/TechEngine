#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

using TechEngine::ComponentRegistry;
using TechEngine::Entity;
using TechEngine::ReadError;
using TechEngine::ResourceId;
using TechEngine::Scene;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;

struct SceneAssetHealth {
    std::uint32_t value = 0;

    template<typename Archive>
    friend void visit(Archive& archive, SceneAssetHealth& health) {
        archive.field(health.value);
    }
};

struct SceneAssetNarrowHealth {
    std::uint16_t value = 0;

    template<typename Archive>
    friend void visit(Archive& archive, SceneAssetNarrowHealth& health) {
        archive.field(health.value);
    }
};

static constexpr ResourceId SCENE_ASSET_ID{0x5EC0FFEE12345678};

static TransformValues makeLocal(const float seed) {
    TransformValues values;
    values.position = Vec3(seed, seed * 2.0f, -seed);
    values.rotation = Transform::fromEulerDegrees(Vec3(seed * 10.0f, seed * 20.0f, seed * 30.0f));
    values.scale = Vec3(1.0f + seed, 2.0f, 0.5f + seed);
    return values;
}

static bool sameLocal(const TransformValues& left, const TransformValues& right) {
    return left.position == right.position && left.rotation == right.rotation && left.scale == right.scale;
}

struct SmallHierarchy {
    Entity root;
    Entity first;
    Entity second;
    Entity third;
    Entity grandchild;
};

// Children are created in one order and linked in another, so a writer that walks storage
// order instead of sibling order fails the round-trip.
static SmallHierarchy buildSmallHierarchy(Scene& scene) {
    SmallHierarchy hierarchy;
    hierarchy.root = scene.createEntity();
    hierarchy.third = scene.createEntity();
    hierarchy.first = scene.createEntity();
    hierarchy.grandchild = scene.createEntity();
    hierarchy.second = scene.createEntity();

    REQUIRE(scene.setParent(hierarchy.first, hierarchy.root, 0));
    REQUIRE(scene.setParent(hierarchy.second, hierarchy.root, 1));
    REQUIRE(scene.setParent(hierarchy.third, hierarchy.root, 2));
    REQUIRE(scene.setParent(hierarchy.grandchild, hierarchy.second, 0));

    REQUIRE(scene.getComponent<Transform>(hierarchy.root).setLocal(makeLocal(1.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.first).setLocal(makeLocal(2.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.second).setLocal(makeLocal(3.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.third).setLocal(makeLocal(4.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.grandchild).setLocal(makeLocal(5.0f)));
    return hierarchy;
}

static std::vector<std::byte> writeSmallHierarchyWithHealth(ComponentRegistry& registry) {
    Scene source(registry);
    const SmallHierarchy hierarchy = buildSmallHierarchy(source);
    source.addComponent<SceneAssetHealth>(hierarchy.first, SceneAssetHealth{77});

    std::vector<std::byte> bytes;
    writeSceneAsset(source, SCENE_ASSET_ID, bytes);
    return bytes;
}

TEST_CASE("Transform carries payload adapters and Hierarchy does not", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);

    const TechEngine::ComponentTypeRecord* transform = registry.find(TechEngine::componentTypeId<Transform>());
    const TechEngine::ComponentTypeRecord* hierarchy = registry.find(TechEngine::componentTypeId<TechEngine::Hierarchy>());

    REQUIRE(transform != nullptr);
    REQUIRE(hierarchy != nullptr);
    CHECK(transform->writePayload != nullptr);
    CHECK(transform->readPayload != nullptr);
    CHECK(hierarchy->writePayload == nullptr);
    CHECK(hierarchy->readPayload == nullptr);
}

TEST_CASE("a small hierarchy round-trips through a scene asset", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    Scene source(writeRegistry);
    const SmallHierarchy written = buildSmallHierarchy(source);

    std::vector<std::byte> bytes;
    writeSceneAsset(source, SCENE_ASSET_ID, bytes);
    REQUIRE(!bytes.empty());

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    Scene loaded(readRegistry);
    ResourceId id;
    const std::error_code readError = readSceneAsset(bytes, loaded, id);

    REQUIRE(!readError);
    REQUIRE(id == SCENE_ASSET_ID);

    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    REQUIRE(loaded.getChildren(children[0]).empty());
    REQUIRE(loaded.getChildren(children[2]).empty());
    const std::vector<Entity> grandchildren = loaded.getChildren(children[1]);
    REQUIRE(grandchildren.size() == 1);
    REQUIRE(loaded.getParent(grandchildren[0]) == children[1]);

    const std::array<Entity, 5> writtenEntities{written.root, written.first, written.second, written.third, written.grandchild};
    const std::array<Entity, 5> readEntities{roots[0], children[0], children[1], children[2], grandchildren[0]};
    for (std::size_t i = 0; i < writtenEntities.size(); i++) {
        const Transform& writtenTransform = source.getComponent<Transform>(writtenEntities[i]);
        const Transform& readTransform = loaded.getComponent<Transform>(readEntities[i]);
        CHECK(sameLocal(writtenTransform.getLocal(), readTransform.getLocal()));
        CHECK(writtenTransform.worldMatrix() == readTransform.worldMatrix());
    }
}

TEST_CASE("an empty scene round-trips through a scene asset", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    const Scene source(registry);

    std::vector<std::byte> bytes;
    writeSceneAsset(source, SCENE_ASSET_ID, bytes);

    Scene loaded(registry);
    ResourceId id;
    const std::error_code readError = readSceneAsset(bytes, loaded, id);

    REQUIRE(!readError);
    REQUIRE(id == SCENE_ASSET_ID);
    REQUIRE(loaded.getRoots().empty());
}

TEST_CASE("every truncation of a scene asset fails soft and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(registry);
    REQUIRE(!bytes.empty());

    for (std::size_t length = 0; length < bytes.size(); length++) {
        INFO("truncated to " << length << " of " << bytes.size() << " bytes");
        Scene loaded(registry);
        ResourceId id;
        const std::error_code readError = readSceneAsset(std::span<const std::byte>{bytes}.first(length), loaded, id);

        REQUIRE(readError == ReadError::Truncated);
        REQUIRE(loaded.getRoots().empty());
    }
}

TEST_CASE("a scene asset of another kind or version is rejected and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene loaded(registry);
    ResourceId id;
    std::vector<std::byte> bytes;
    TechEngine::Writer writer{bytes};

    SECTION("another kind's magic") {
        writer.writeHeader(TechEngine::BlobKind{TechEngine::SCENE_KIND.magic + 1, TechEngine::SCENE_KIND.version});

        REQUIRE(readSceneAsset(bytes, loaded, id) == ReadError::BadMagic);
    }

    SECTION("another version") {
        writer.writeHeader(TechEngine::BlobKind{TechEngine::SCENE_KIND.magic, static_cast<std::uint16_t>(TechEngine::SCENE_KIND.version + 1)});

        REQUIRE(readSceneAsset(bytes, loaded, id) == ReadError::BadVersion);
    }

    REQUIRE(loaded.getRoots().empty());
}

TEST_CASE("a block with an unregistered tag is skipped and the rest of the asset loads", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    writeRegistry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(writeRegistry);

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    Scene loaded(readRegistry);
    ResourceId id;
    const std::error_code readError = readSceneAsset(bytes, loaded, id);

    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    CHECK(sameLocal(loaded.getComponent<Transform>(children[0]).getLocal(), makeLocal(2.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[1]).getLocal(), makeLocal(3.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[2]).getLocal(), makeLocal(4.0f)));
}

TEST_CASE("a registered block whose payload does not fill its length is skipped", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    writeRegistry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(writeRegistry);

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    readRegistry.registerComponent<SceneAssetNarrowHealth>("Tests.SceneAssetHealth");
    Scene loaded(readRegistry);
    ResourceId id;
    const std::error_code readError = readSceneAsset(bytes, loaded, id);

    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    CHECK(!loaded.hasComponent<SceneAssetNarrowHealth>(children[0]));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[0]).getLocal(), makeLocal(2.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[1]).getLocal(), makeLocal(3.0f)));
}

TEST_CASE("reading into a Scene that already holds entities is a programmer error", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    const Scene source(registry);
    std::vector<std::byte> bytes;
    writeSceneAsset(source, SCENE_ASSET_ID, bytes);

    Scene loaded(registry);
    loaded.createEntity();
    ResourceId id;

    const TechEngineTests::FatalAssertGuard guard;
    REQUIRE_THROWS_AS(readSceneAsset(bytes, loaded, id), TechEngineTests::AssertFired);
}

struct SceneAssetRow {
    std::uint32_t authoringId = 0;
    std::uint32_t parentId = 0;
};

static constexpr std::uint32_t SCENE_ASSET_NO_PARENT = 0xFFFFFFFF;

static std::vector<std::byte> writeRows(const std::vector<SceneAssetRow>& rows) {
    std::vector<std::byte> bytes;
    TechEngine::Writer writer{bytes};
    writer.writeHeader(TechEngine::SCENE_KIND);
    writer.write(SCENE_ASSET_ID.value());
    writer.write(static_cast<std::uint32_t>(rows.size()));
    for (const SceneAssetRow& row: rows) {
        writer.write(row.authoringId);
        writer.write(row.parentId);
        writer.write(static_cast<std::uint32_t>(0));
    }
    return bytes;
}

TEST_CASE("a scene asset with a broken entity table is Malformed and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene loaded(registry);
    ResourceId id;
    std::vector<std::byte> bytes;

    SECTION("a parent id that names no entity") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}, {1, 7}});
    }

    SECTION("a parent that comes after its child") {
        bytes = writeRows({{0, 1}, {1, SCENE_ASSET_NO_PARENT}});
    }

    SECTION("an entity that is its own parent") {
        bytes = writeRows({{0, 0}});
    }

    SECTION("an authoring id that is not its row") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}, {2, 0}});
    }

    SECTION("bytes after the last entity") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}});
        bytes.push_back(std::byte{0});
    }

    REQUIRE(readSceneAsset(bytes, loaded, id) == ReadError::Malformed);
    REQUIRE(loaded.getRoots().empty());
    REQUIRE(id == ResourceId{});
}
