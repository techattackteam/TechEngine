#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/ArchetypeStorage.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
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

static void requireVectorNear(const TechEngine::Vec3& actual, const TechEngine::Vec3& expected) {
    REQUIRE(actual.x == Catch::Approx(expected.x).margin(0.0001f));
    REQUIRE(actual.y == Catch::Approx(expected.y).margin(0.0001f));
    REQUIRE(actual.z == Catch::Approx(expected.z).margin(0.0001f));
}

static void requireMatrixNear(const TechEngine::Mat4& actual, const TechEngine::Mat4& expected) {
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            CHECK(actual[column][row] == Catch::Approx(expected[column][row]).margin(0.0001f));
        }
    }
}

static bool applyWorldEdit(TechEngine::Scene& scene, const TechEngine::Entity entity, const TechEngine::TransformValues& world) {
    TechEngine::TransformValues local;
    return scene.fromWorldToLocal(entity, world, local) && scene.getComponent<TechEngine::Transform>(entity).setLocal(local);
}

TEST_CASE("new entities carry identity transforms", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
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

TEST_CASE("transform values survive archetype transitions", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
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

TEST_CASE("propagation follows parent-first order after local writes", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity leaf = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    const TechEngine::Entity root = scene.createEntity();
    REQUIRE(scene.setParent(child, root));
    REQUIRE(scene.setParent(leaf, child));
    REQUIRE(scene.getComponent<TechEngine::Transform>(root).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(0.0f, 2.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(leaf).setLocal(values(TechEngine::Vec3(0.0f, 0.0f, 3.0f))));

    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(root).getWorld().position, TechEngine::Vec3(10.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(10.0f, 2.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(10.0f, 2.0f, 3.0f));

    REQUIRE(scene.getComponent<TechEngine::Transform>(root).setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(leaf).getWorld().position, TechEngine::Vec3(20.0f, 2.0f, 3.0f));
}

TEST_CASE("propagation composes parent rotation and scale", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
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

TEST_CASE("preserve-local parenting changes world values after propagation", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity firstParent = scene.createEntity();
    const TechEngine::Entity secondParent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(firstParent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(secondParent).setLocal(values(TechEngine::Vec3(20.0f, 0.0f, 0.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f))));
    REQUIRE(scene.setParent(child, firstParent));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(12.0f, 0.0f, 0.0f));

    REQUIRE(scene.setParent(child, secondParent, 0, TechEngine::ReparentMode::PreserveLocal));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(12.0f, 0.0f, 0.0f));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(22.0f, 0.0f, 0.0f));

    REQUIRE(scene.unparent(child, TechEngine::ReparentMode::PreserveLocal));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
}

TEST_CASE("preserve-world reparenting and detaching keep the rendered transform", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity firstParent = scene.createEntity();
    const TechEngine::Entity secondParent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.getComponent<TechEngine::Transform>(firstParent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(2.0f, 0.0f, 0.0f), rotationAroundZ(30.0f))));
    REQUIRE(scene.setParent(child, firstParent));
    scene.propagateTransforms();
    const TechEngine::Mat4 before = scene.getComponent<TechEngine::Transform>(child).worldMatrix();

    REQUIRE(scene.getComponent<TechEngine::Transform>(secondParent).setLocal(values(TechEngine::Vec3(-4.0f, 0.0f, 0.0f), rotationAroundZ(-90.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(scene.setParent(child, secondParent, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE(scene.getParent(child) == secondParent);
    scene.propagateTransforms();
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);

    REQUIRE(scene.unparent(child, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE_FALSE(scene.getParent(child).valid());
    scene.propagateTransforms();
    requireMatrixNear(scene.getComponent<TechEngine::Transform>(child).worldMatrix(), before);
}

TEST_CASE("failed preserve-world reparenting leaves topology and transforms intact", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
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

TEST_CASE("world edits use the current parent state before propagation", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f))));

    REQUIRE(applyWorldEdit(scene, child, values(TechEngine::Vec3(14.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(4.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().scale, TechEngine::Vec3(2.0f));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(14.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().scale, TechEngine::Vec3(4.0f));

    REQUIRE(applyWorldEdit(scene, parent, values(TechEngine::Vec3(20.0f, 0.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f))));
    REQUIRE(applyWorldEdit(scene, child, values(TechEngine::Vec3(24.0f, 0.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(parent).getLocal().position, TechEngine::Vec3(20.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(parent).getWorld().position, TechEngine::Vec3(20.0f, 0.0f, 0.0f));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(24.0f, 0.0f, 0.0f));
}

TEST_CASE("world edits invert the current parent rotation", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(10.0f, 0.0f, 0.0f), rotationAroundZ(90.0f), TechEngine::Vec3(2.0f))));

    REQUIRE(applyWorldEdit(scene, child, values(TechEngine::Vec3(10.0f, 4.0f, 0.0f))));
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getLocal().position, TechEngine::Vec3(2.0f, 0.0f, 0.0f));
    scene.propagateTransforms();
    requireVectorNear(scene.getComponent<TechEngine::Transform>(child).getWorld().position, TechEngine::Vec3(10.0f, 4.0f, 0.0f));
}

TEST_CASE("world edits reject zero scale and stale entity lookup", "[core][scene][transform]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    const TechEngine::TransformValues invalid = values(TechEngine::Vec3(1.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(1.0f, 0.0f, 1.0f));
    REQUIRE_FALSE(applyWorldEdit(scene, entity, invalid));
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).getLocal().scale == TechEngine::Vec3(1.0f));

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
    REQUIRE(scene.getComponent<TechEngine::Transform>(replacement).getLocal().position == TechEngine::Vec3(0.0f));
    scene.clear();
    REQUIRE_FALSE(scene.hasComponent<TechEngine::Transform>(replacement));
    REQUIRE_THROWS_AS(scene.getComponent<TechEngine::Transform>(replacement), TechEngineTests::AssertFired);
}

TEST_CASE("non-uniform parent scale preserves shear in the world matrix", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent));
    REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(values(TechEngine::Vec3(0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(2.0f, 1.0f, 1.0f))));
    REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(values(TechEngine::Vec3(1.0f, 0.0f, 0.0f), rotationAroundZ(45.0f))));

    scene.propagateTransforms();
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
}

TEST_CASE("deep transform propagation does not depend on recursion depth", "[core][scene][transform]") {
    TechEngine::ComponentRegistry registry;
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
