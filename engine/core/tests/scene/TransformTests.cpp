#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/ArchetypeStorage.hpp>
#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

static_assert(TechEngine::ComponentValue<TechEngine::Transform>);

struct TransformMarker {
    int value = 0;
};

static TechEngine::TransformValues values(const TechEngine::Vec3 position, const TechEngine::Quat rotation = TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), const TechEngine::Vec3 scale = TechEngine::Vec3(1.0f)) {
    return {position, rotation, scale};
}

static TechEngine::Quat rotationAroundZ(const float degrees) {
    const float halfAngle = degrees * std::numbers::pi_v<float> / 360.0f;
    return TechEngine::Quat(std::cos(halfAngle), 0.0f, 0.0f, std::sin(halfAngle));
}

static void requireVectorNear(const TechEngine::Vec3& actual, const TechEngine::Vec3& expected, const float margin = 0.0001f) {
    REQUIRE(actual.x == Catch::Approx(expected.x).margin(margin));
    REQUIRE(actual.y == Catch::Approx(expected.y).margin(margin));
    REQUIRE(actual.z == Catch::Approx(expected.z).margin(margin));
}

static void requireMatrixNear(const TechEngine::Mat4& actual, const TechEngine::Mat4& expected, const float margin = 0.0001f) {
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            CHECK(actual[column][row] == Catch::Approx(expected[column][row]).margin(margin));
        }
    }
}

TEST_CASE("new entities carry identity transforms", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    const TechEngine::Transform* transform = storage.component<TechEngine::Transform>(entity);

    REQUIRE(transform != nullptr);
    REQUIRE(transform->getLocal().position == TechEngine::Vec3(0.0f));
    REQUIRE(transform->getLocal().rotation == TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(transform->getLocal().scale == TechEngine::Vec3(1.0f));
    REQUIRE(transform->getWorld().position == TechEngine::Vec3(0.0f));
    REQUIRE(transform->getWorld().rotation == TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f));
    REQUIRE(transform->getWorld().scale == TechEngine::Vec3(1.0f));
    REQUIRE(transform->worldMatrix() == TechEngine::Mat4(1.0f));
}

TEST_CASE("Scene getComponent gives typed mutable and const access", "[core][scene][transform]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<TransformMarker>("Tests.TransformMarker");
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    TechEngine::Transform& transform = scene.getComponent<TechEngine::Transform>(entity);
    const TechEngine::Scene& readOnlyScene = scene;

    REQUIRE(&readOnlyScene.getComponent<TechEngine::Transform>(entity) == &transform);
    REQUIRE(scene.hasComponent<TechEngine::Transform>(entity));
    REQUIRE_FALSE(scene.hasComponent<TransformMarker>(entity));
    REQUIRE_THROWS_AS(scene.getComponent<TransformMarker>(entity), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(readOnlyScene.getComponent<TransformMarker>(entity), TechEngineTests::AssertFired);

    REQUIRE(scene.destroyEntity(entity));
    REQUIRE_FALSE(scene.hasComponent<TechEngine::Transform>(entity));
    REQUIRE_THROWS_AS(scene.getComponent<TechEngine::Transform>(entity), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(readOnlyScene.getComponent<TechEngine::Transform>(entity), TechEngineTests::AssertFired);
}

TEST_CASE("a copied bound transform cannot write through to its source entity", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));
    TechEngine::Transform copy = scene.getComponent<TechEngine::Transform>(entity);

    REQUIRE_FALSE(copy.setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f))));
    REQUIRE_FALSE(copy.setWorld(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(entity).getWorld().position, TechEngine::Vec3(1.0f, 0.0f, 0.0f));
}

TEST_CASE("assigning a bound transform keeps its entity binding and updates descendants", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity sourceEntity = scene.createEntity();
    const TechEngine::Entity destinationEntity = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, destinationEntity));

    TechEngine::Transform& source = scene.getComponent<TechEngine::Transform>(sourceEntity);
    TechEngine::Transform& destination = scene.getComponent<TechEngine::Transform>(destinationEntity);
    REQUIRE(source.setLocal(values(TechEngine::Vec3(4.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(destination.setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));

    destination = source;
    requireVectorNear(destination.getLocal().position, TechEngine::Vec3(4.0f, 0.0f, 0.0f));
    requireVectorNear(destination.getWorld().position, TechEngine::Vec3(4.0f, 0.0f, 0.0f));
    requireVectorNear(destination.getLocal().scale, TechEngine::Vec3(2.0f));
    requireVectorNear(destination.getLocal().rotation * TechEngine::Vec3(1.0f, 0.0f, 0.0f), TechEngine::Vec3(0.0f, 1.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(4.0f, 2.0f, 0.0f));
    requireVectorNear(source.getWorld().position, TechEngine::Vec3(4.0f, 0.0f, 0.0f));

    TechEngine::Transform detachedCopy;
    detachedCopy = source;
    REQUIRE_FALSE(detachedCopy.setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    requireVectorNear(source.getWorld().position, TechEngine::Vec3(4.0f, 0.0f, 0.0f));

    REQUIRE(source.setLocal(values(TechEngine::Vec3(6.0f, 0.0f, 0.0f), rotationAroundZ(-90.0f), TechEngine::Vec3(3.0f))));
    destination = std::move(source);
    requireVectorNear(destination.getWorld().position, TechEngine::Vec3(6.0f, 0.0f, 0.0f));
    requireVectorNear(destination.getWorld().scale, TechEngine::Vec3(3.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(6.0f, -3.0f, 0.0f));

    REQUIRE(destination.setLocal(values(TechEngine::Vec3(8.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(9.0f, 0.0f, 0.0f));
}

TEST_CASE("assignment between scenes keeps the destination scene binding", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene sourceScene(registry);
    TechEngine::Scene destinationScene(registry);
    const TechEngine::Entity sourceEntity = sourceScene.createEntity();
    const TechEngine::Entity destinationEntity = destinationScene.createEntity();
    REQUIRE(sourceScene.getComponent<TechEngine::Transform>(sourceEntity).setLocal(values(TechEngine::Vec3(3.0f, 0.0f, 0.0f))));

    destinationScene.getComponent<TechEngine::Transform>(destinationEntity) = sourceScene.getComponent<TechEngine::Transform>(sourceEntity);
    REQUIRE(destinationScene.getComponent<TechEngine::Transform>(destinationEntity).setLocal(values(TechEngine::Vec3(7.0f, 0.0f, 0.0f))));
    requireVectorNear(destinationScene.getComponent<TechEngine::Transform>(destinationEntity).getWorld().position, TechEngine::Vec3(7.0f, 0.0f, 0.0f));
    requireVectorNear(sourceScene.getComponent<TechEngine::Transform>(sourceEntity).getWorld().position, TechEngine::Vec3(3.0f, 0.0f, 0.0f));
}

TEST_CASE("swapping a component storage row retains the surviving transform binding", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity removed = scene.createEntity();
    const TechEngine::Entity survivor = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(survivor).setLocal(values(TechEngine::Vec3(3.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));

    REQUIRE(scene.destroyEntity(removed));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(survivor).getLocal().position, TechEngine::Vec3(3.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(survivor).getWorld().position, TechEngine::Vec3(3.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(survivor).getLocal().scale, TechEngine::Vec3(2.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(survivor).getLocal().rotation * TechEngine::Vec3(1.0f, 0.0f, 0.0f), TechEngine::Vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(scene.getComponent<TechEngine::Transform>(survivor).setLocal(values(TechEngine::Vec3(5.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(survivor).getWorld().position, TechEngine::Vec3(5.0f, 0.0f, 0.0f));
}

TEST_CASE("transform binding survives component storage growth", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity first = scene.createEntity();
    for (int index = 0; index < 64; index++) {
        scene.createEntity();
    }

    REQUIRE(scene.getComponent<TechEngine::Transform>(first).setWorld(values(TechEngine::Vec3(5.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(first).getWorld().position, TechEngine::Vec3(5.0f, 0.0f, 0.0f));
}

TEST_CASE("transform values survive archetype transitions", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<TransformMarker>("Tests.TransformMarker");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    const TechEngine::TransformValues local = values(TechEngine::Vec3(3.0f, 4.0f, 5.0f), rotationAroundZ(30.0f), TechEngine::Vec3(2.0f));
    REQUIRE(storage.component<TechEngine::Transform>(entity)->setLocal(local));

    REQUIRE(storage.addComponent(entity, TransformMarker{7}));
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().position == local.position);
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().rotation == local.rotation);
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().scale == local.scale);
    REQUIRE(storage.removeComponent<TransformMarker>(entity));
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().position == local.position);
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().rotation == local.rotation);
    REQUIRE(storage.component<TechEngine::Transform>(entity)->getLocal().scale == local.scale);
}

TEST_CASE("euler display conversion preserves quaternion orientation", "[core][scene][transform]") {
    const TechEngine::Quat rotation = TechEngine::Transform::fromEulerDegrees(TechEngine::Vec3(0.0f, 60.0f, 0.0f));
    const TechEngine::Vec3 turned = rotation * TechEngine::Vec3(1.0f, 0.0f, 0.0f);
    requireVectorNear(turned, TechEngine::Vec3(0.5f, 0.0f, -std::sqrt(3.0f) / 2.0f));

    const TechEngine::Quat roundTrip = TechEngine::Transform::fromEulerDegrees(TechEngine::Transform::toEulerDegrees(rotation));
    requireVectorNear(roundTrip * TechEngine::Vec3(1.0f, 0.0f, 0.0f), turned);
}

TEST_CASE("zero scale is rejected without changing the local transform", "[core][scene][transform]") {
    TechEngine::Transform transform;
    const TechEngine::TransformValues valid = values(TechEngine::Vec3(1.0f, 2.0f, 3.0f), rotationAroundZ(30.0f), TechEngine::Vec3(2.0f, 3.0f, 4.0f));
    REQUIRE(transform.setLocal(valid));

    for (int axis = 0; axis < 3; axis++) {
        TechEngine::TransformValues invalid = valid;
        invalid.scale[axis] = 0.0f;
        REQUIRE_FALSE(transform.setLocal(invalid));
        REQUIRE(transform.getLocal().position == valid.position);
        REQUIRE(transform.getLocal().rotation == valid.rotation);
        REQUIRE(transform.getLocal().scale == valid.scale);
    }
}

TEST_CASE("small nonzero local scales are accepted on every axis", "[core][scene][transform]") {
    TechEngine::Transform transform;
    for (int axis = 0; axis < 3; axis++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            TechEngine::TransformValues small = values(TechEngine::Vec3(1.0f, 2.0f, 3.0f));
            small.scale[axis] = static_cast<float>(sign) * 0.000001f;
            REQUIRE(transform.setLocal(small));
            REQUIRE(transform.getLocal().scale == small.scale);

            TechEngine::TransformValues zero = small;
            zero.scale[axis] = 0.0f;
            REQUIRE_FALSE(transform.setLocal(zero));
            REQUIRE(transform.getLocal().position == small.position);
            REQUIRE(transform.getLocal().scale == small.scale);
        }
    }
}

TEST_CASE("local writes immediately propagate through descendants", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity leaf = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::Entity root = scene.createEntity();
    REQUIRE(scene.setParent(child, root));
    REQUIRE(scene.setParent(leaf, child));
    REQUIRE(scene.getComponent<TechEngine::Transform>(root).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(0.0f, 2.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(leaf).setLocal(values(TechEngine::Vec3(0.0f, 0.0f, 3.0f))));

    requireVectorNear(scene.getComponent<TechEngine::Transform>(root).getWorld().position, TechEngine::Vec3(10.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(10.0f, 2.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(10.0f, 2.0f, 3.0f));

    REQUIRE(scene.getComponent<TechEngine::Transform>(root).setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(20.0f, 2.0f, 3.0f));
}

TEST_CASE("world edits after an ancestor write use current transforms and update descendants", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::Entity leaf = scene.createEntity();
    REQUIRE(scene.setParent(child, root));
    REQUIRE(scene.setParent(leaf, child));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(leaf).setLocal(values(TechEngine::Vec3(3.0f, 0.0f, 0.0f))));

    REQUIRE(scene.getComponent<TechEngine::Transform>(root).setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(25.0f, 0.0f, 0.0f));

    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setWorld(values(TechEngine::Vec3(24.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(4.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(27.0f, 0.0f, 0.0f));
}

TEST_CASE("propagation composes parent rotation and scale", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));

    scene.propagateTransforms();
    const TechEngine::Transform* transform = &scene.getComponent<TechEngine::Transform>(child);
    requireVectorNear(transform->getWorld().position, TechEngine::Vec3(10.0f, 2.0f, 0.0f));
    requireVectorNear(transform->getWorld().scale, TechEngine::Vec3(2.0f));
    requireVectorNear(transform->getWorld().rotation * TechEngine::Vec3(1.0f, 0.0f, 0.0f), TechEngine::Vec3(0.0f, 1.0f, 0.0f));
    const TechEngine::Vec4 worldPoint = transform->worldMatrix() * TechEngine::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    requireVectorNear(TechEngine::Vec3(worldPoint), TechEngine::Vec3(10.0f, 4.0f, 0.0f));
}

TEST_CASE("propagation keeps small nonzero world scales", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(0.001f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(0.001f))));

    scene.propagateTransforms();
    const TechEngine::Transform& transform = scene.getComponent<TechEngine::Transform>(child);
    for (int axis = 0; axis < 3; axis++) {
        REQUIRE(transform.getWorld().scale[axis] == Catch::Approx(0.000001f).margin(0.0000001f));
    }
    REQUIRE(transform.worldMatrix()[0][0] == Catch::Approx(0.000001f).margin(0.0000001f));
}

TEST_CASE("preserve-world detach accepts small nonzero world scale", "[core][scene][transform]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(0.001f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(0.001f))));
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();
    REQUIRE(before[0][0] == Catch::Approx(0.000001f).margin(0.000000001f));

    bool detached = false;
    CHECK_NOTHROW(detached = scene.unparent(child, TechEngine::ReparentMode::PreserveWorld));
    CHECK(detached);
    REQUIRE_FALSE(scene.getParent(child).valid());
    REQUIRE(scene.getChildren(parent).empty());
    const TechEngine::Transform& transform = scene.getComponent<TechEngine::Transform>(child);
    requireMatrixNear(transform.worldMatrix(), before, 0.000000001f);
    requireVectorNear(transform.getLocal().scale, TechEngine::Vec3(0.000001f), 0.000000001f);
    requireVectorNear(transform.getWorld().scale, TechEngine::Vec3(0.000001f), 0.000000001f);

    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(4.0f, 0.0f, 0.0f))));
    requireMatrixNear(transform.worldMatrix(), before, 0.000000001f);
}

TEST_CASE("preserve-world reparenting accepts small nonzero local scale", "[core][scene][transform]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(100000.0f))));
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    bool reparented = false;
    CHECK_NOTHROW(reparented = scene.setParent(child, parent, 0, TechEngine::ReparentMode::PreserveWorld));
    CHECK(reparented);
    REQUIRE(scene.getParent(child) == parent);
    REQUIRE(scene.getChildren(parent) == std::vector<TechEngine::Entity>{child});
    const TechEngine::Transform& transform = scene.getComponent<TechEngine::Transform>(child);
    requireMatrixNear(transform.worldMatrix(), before, 0.00001f);
    requireVectorNear(transform.getLocal().scale, TechEngine::Vec3(0.00001f), 0.00000001f);
    requireVectorNear(transform.getWorld().scale, TechEngine::Vec3(1.0f));

    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(100000.0f))));
    requireVectorNear(transform.getWorld().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(transform.getWorld().scale, TechEngine::Vec3(1.0f));
}

TEST_CASE("preserve-local parenting immediately updates world values", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity firstParent = scene.createEntity();
    const TechEngine::Entity secondParent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(firstParent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(secondParent).setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f))));
    REQUIRE(scene.setParent(child, firstParent));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(12.0f, 0.0f, 0.0f));

    REQUIRE(scene.setParent(child, secondParent, 0, TechEngine::ReparentMode::PreserveLocal));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(22.0f, 0.0f, 0.0f));

    REQUIRE(scene.unparent(child, TechEngine::ReparentMode::PreserveLocal));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
}

TEST_CASE("preserve-world reparenting and detaching keep the rendered transform", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity firstParent = scene.createEntity();
    const TechEngine::Entity secondParent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(firstParent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f), rotationAroundZ(30.0f))));
    REQUIRE(scene.setParent(child, firstParent));
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    REQUIRE(scene.getComponent<TechEngine::Transform>(secondParent).setLocal(values(TechEngine::Vec3(-4.0f, 0.0f, 0.0f), rotationAroundZ(-90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.setParent(child, secondParent, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE(scene.getParent(child) == secondParent);
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);

    REQUIRE(scene.unparent(child, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE_FALSE(scene.getParent(child).valid());
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("failed preserve-world reparenting leaves topology and transforms intact", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::Entity stale = scene.createEntity();
    REQUIRE(scene.setParent(child, root));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(3.0f, 0.0f, 0.0f))));
    scene.propagateTransforms();
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();
    REQUIRE(scene.destroyEntity(stale));

    REQUIRE_FALSE(scene.setParent(child, stale, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE_FALSE(scene.setParent(root, child, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE_FALSE(scene.setParent(child, root, 1, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE(scene.getParent(child) == root);
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(3.0f, 0.0f, 0.0f));
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("preserve-world reparenting rejects unrepresentable shear", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::TransformValues local = values(TechEngine::Vec3(1.0f, 2.0f, 0.0f), rotationAroundZ(45.0f));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(3.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f, 1.0f, 1.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(local));
    scene.propagateTransforms();
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    REQUIRE_FALSE(scene.setParent(child, parent, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE_FALSE(scene.getParent(child).valid());
    REQUIRE(scene.getChildren(parent).empty());
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, local.position);
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).getLocal().rotation == local.rotation);
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().scale, local.scale);
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("preserve-world detach rejects a sheared world transform", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::TransformValues local = values(TechEngine::Vec3(1.0f, 0.0f, 0.0f), rotationAroundZ(45.0f));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f, 1.0f, 1.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(local));
    REQUIRE(scene.setParent(child, parent));
    scene.propagateTransforms();
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    REQUIRE_FALSE(scene.unparent(child, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE(scene.getParent(child) == parent);
    REQUIRE(scene.getChildren(parent) == std::vector<TechEngine::Entity>{child});
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, local.position);
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).getLocal().rotation == local.rotation);
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().scale, local.scale);
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("world edits use the current parent state", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f))));

    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setWorld(values(TechEngine::Vec3(14.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(4.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().scale, TechEngine::Vec3(2.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(14.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().scale, TechEngine::Vec3(4.0f));

    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setWorld(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setWorld(values(TechEngine::Vec3(24.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(parent).getLocal().position, TechEngine::Vec3(20.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(parent).getWorld().position, TechEngine::Vec3(20.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(24.0f, 0.0f, 0.0f));
}

TEST_CASE("world edits invert the current parent rotation", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));

    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setWorld(values(TechEngine::Vec3(10.0f, 4.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(10.0f, 4.0f, 0.0f));
}

TEST_CASE("world edits reject zero scale and stale entity lookup", "[core][scene][transform]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    const TechEngine::TransformValues invalid = values(TechEngine::Vec3(1.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(1.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(scene.getComponent<TechEngine::Transform>(entity).setWorld(invalid));
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).getLocal().scale == TechEngine::Vec3(1.0f));
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).worldMatrix() == TechEngine::Mat4(1.0f));

    REQUIRE(scene.destroyEntity(entity));
    REQUIRE_FALSE(scene.hasComponent<TechEngine::Transform>(entity));
    TechEngine::Mat4 staleWorld(2.0f);
    REQUIRE_FALSE(scene.fromLocalToWorld(entity, staleWorld));
    REQUIRE(staleWorld == TechEngine::Mat4(2.0f));
    TechEngine::TransformValues staleLocal = values(TechEngine::Vec3(3.0f));
    REQUIRE_FALSE(scene.fromWorldToLocal(entity, values(TechEngine::Vec3(2.0f)), staleLocal));
    REQUIRE(staleLocal.position == TechEngine::Vec3(3.0f));
    REQUIRE_THROWS_AS(scene.getComponent<TechEngine::Transform>(entity), TechEngineTests::AssertFired);
    const TechEngine::Entity replacement = scene.createEntity();
    REQUIRE(scene.hasComponent<TechEngine::Transform>(replacement));
    REQUIRE_THROWS_AS(scene.getComponent<TechEngine::Transform>(entity), TechEngineTests::AssertFired);
    REQUIRE(scene.getComponent<TechEngine::Transform>(replacement).setWorld(values(TechEngine::Vec3(3.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(replacement).getLocal().position == TechEngine::Vec3(3.0f));
    scene.clear();
    REQUIRE_FALSE(scene.hasComponent<TechEngine::Transform>(replacement));
    REQUIRE_THROWS_AS(scene.getComponent<TechEngine::Transform>(replacement), TechEngineTests::AssertFired);
}

TEST_CASE("non-uniform parent scale preserves shear in the world matrix", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f, 1.0f, 1.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f), rotationAroundZ(45.0f))));

    const TechEngine::Transform* transform = &scene.getComponent<TechEngine::Transform>(child);
    TechEngine::Mat4 computedWorld;
    REQUIRE(scene.fromLocalToWorld(child, computedWorld));
    requireMatrixNear(transform->worldMatrix(), computedWorld);
    const float diagonal = std::sqrt(0.5f);
    REQUIRE(transform->worldMatrix()[0][0] == Catch::Approx(2.0f * diagonal).margin(0.0001f));
    REQUIRE(transform->worldMatrix()[0][1] == Catch::Approx(diagonal).margin(0.0001f));
    REQUIRE(transform->worldMatrix()[1][0] == Catch::Approx(-2.0f * diagonal).margin(0.0001f));
    REQUIRE(transform->worldMatrix()[1][1] == Catch::Approx(diagonal).margin(0.0001f));
    requireVectorNear(transform->getWorld().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    REQUIRE(std::isfinite(transform->getWorld().scale.x));
    REQUIRE(std::isfinite(transform->getWorld().scale.y));
    REQUIRE(std::isfinite(transform->getWorld().rotation.w));
    REQUIRE(std::isfinite(transform->getWorld().rotation.z));
    REQUIRE(transform->getWorld().scale.x != 0.0f);
    REQUIRE(transform->getWorld().scale.y != 0.0f);

    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(3.0f, 1.0f, 1.0f))));
    REQUIRE(scene.fromLocalToWorld(child, computedWorld));
    requireMatrixNear(transform->worldMatrix(), computedWorld);
    requireVectorNear(transform->getWorld().position, TechEngine::Vec3(3.0f, 0.0f, 0.0f));
}

TEST_CASE("world edits reject local shear without changing the transform", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f, 1.0f, 1.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    REQUIRE_FALSE(scene.getComponent<TechEngine::Transform>(child).setWorld(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f), rotationAroundZ(45.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(1.0f, 0.0f, 0.0f));
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("deep transform propagation does not depend on recursion depth", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    TechEngine::Entity parent = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));

    for (int depth = 1; depth < 4096; depth++) {
        const TechEngine::Entity child = scene.createEntity();
        REQUIRE(scene.setParent(child, parent));
        REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f))));
        parent = child;
    }

    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(parent).getWorld().position, TechEngine::Vec3(4096.0f, 0.0f, 0.0f));
}
