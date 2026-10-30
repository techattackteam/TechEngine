#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using TechEngine::ComponentTypeId;
using TechEngine::Entity;
using TechEngine::Scene;

struct SceneRawComponent {
    std::uint32_t value = 7;
};

struct SceneRawOther {
    std::uint32_t value = 0;
};

class SceneRawFixture {
private:
    TechEngine::ComponentRegistry m_registry;

public:
    Scene scene{registerComponents(m_registry)};
    ComponentTypeId rawType = TechEngine::componentTypeId<SceneRawComponent>();
    ComponentTypeId otherType = TechEngine::componentTypeId<SceneRawOther>();

private:
    static TechEngine::ComponentRegistry& registerComponents(TechEngine::ComponentRegistry& registry) {
        TechEngineTests::registerBuiltInSceneComponents(registry);
        registry.registerComponent<SceneRawComponent>("Tests.SceneRawComponent");
        registry.registerComponent<SceneRawOther>("Tests.SceneRawOther");
        return registry;
    }
};

TEST_CASE("componentRaw finds a component the entity holds, and nothing for one it lacks or for a dead entity", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity entity = scene.createEntity();
    scene.addComponent<SceneRawComponent>(entity);

    CHECK(scene.componentRaw(entity, fixture.rawType) == &scene.getComponent<SceneRawComponent>(entity));
    CHECK(scene.componentRaw(entity, fixture.otherType) == nullptr);

    REQUIRE(scene.destroyEntity(entity));
    CHECK(scene.componentRaw(entity, fixture.rawType) == nullptr);
}

TEST_CASE("addComponentRaw gives a live entity a default-constructed component and keeps its other components", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity entity = scene.createEntity();
    scene.getComponent<TechEngine::Name>(entity).value = "Kept";
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).setLocal({TechEngine::Vec3(1.0f, 2.0f, 3.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(1.0f)}));

    REQUIRE(scene.addComponentRaw(entity, fixture.rawType));

    REQUIRE(scene.hasComponent<SceneRawComponent>(entity));
    CHECK(scene.getComponent<SceneRawComponent>(entity).value == 7);
    CHECK_FALSE(scene.hasComponent<SceneRawOther>(entity));
    CHECK(scene.getComponent<TechEngine::Name>(entity).value == "Kept");
    CHECK(scene.getComponent<TechEngine::Transform>(entity).getLocal().position == TechEngine::Vec3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("addComponentRaw refuses a type the entity already holds and a dead entity, and changes nothing", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity entity = scene.createEntity();
    scene.addComponent<SceneRawComponent>(entity);
    scene.getComponent<SceneRawComponent>(entity).value = 11;

    SECTION("a type the entity already holds") {
        CHECK_FALSE(scene.addComponentRaw(entity, fixture.rawType));
        CHECK(scene.getComponent<SceneRawComponent>(entity).value == 11);
    }

    SECTION("a dead entity") {
        const Entity dead = scene.createEntity();
        REQUIRE(scene.destroyEntity(dead));

        CHECK_FALSE(scene.addComponentRaw(dead, fixture.otherType));
        CHECK_FALSE(scene.contains(dead));
    }
}

TEST_CASE("removeComponentRaw takes the component off, keeps the others and refuses one the entity lacks", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity entity = scene.createEntity();
    scene.getComponent<TechEngine::Name>(entity).value = "Kept";
    REQUIRE(scene.getComponent<TechEngine::Transform>(entity).setLocal({TechEngine::Vec3(1.0f, 2.0f, 3.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), TechEngine::Vec3(1.0f)}));
    scene.addComponent<SceneRawComponent>(entity);
    scene.addComponent<SceneRawOther>(entity);
    scene.getComponent<SceneRawOther>(entity).value = 5;

    REQUIRE(scene.removeComponentRaw(entity, fixture.rawType));
    CHECK_FALSE(scene.hasComponent<SceneRawComponent>(entity));
    CHECK(scene.contains(entity));
    REQUIRE(scene.hasComponent<SceneRawOther>(entity));
    CHECK(scene.getComponent<SceneRawOther>(entity).value == 5);
    CHECK(scene.getComponent<TechEngine::Name>(entity).value == "Kept");
    CHECK(scene.getComponent<TechEngine::Transform>(entity).getLocal().position == TechEngine::Vec3(1.0f, 2.0f, 3.0f));

    CHECK_FALSE(scene.removeComponentRaw(entity, fixture.rawType));

    const Entity dead = scene.createEntity();
    REQUIRE(scene.destroyEntity(dead));
    CHECK_FALSE(scene.removeComponentRaw(dead, fixture.otherType));
}

TEST_CASE("the raw add and remove refuse a type the registry does not know", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity entity = scene.createEntity();
    const ComponentTypeId unknown{TechEngine::StringId("Tests.SceneRawUnregistered")};

    CHECK_FALSE(scene.addComponentRaw(entity, unknown));
    CHECK_FALSE(scene.removeComponentRaw(entity, unknown));
    CHECK(scene.componentRaw(entity, unknown) == nullptr);
    CHECK(scene.contains(entity));
}

TEST_CASE("the raw add and remove refuse Hierarchy, Transform and Name, which every entity keeps", "[core][scene]") {
    SceneRawFixture fixture;
    Scene& scene = fixture.scene;
    const Entity parent = scene.createEntity();
    const Entity entity = scene.createEntity();
    REQUIRE(scene.setParent(entity, parent));
    scene.getComponent<TechEngine::Name>(entity).value = "Kept";

    const ComponentTypeId builtIns[] = {
        TechEngine::componentTypeId<TechEngine::Hierarchy>(),
        TechEngine::componentTypeId<TechEngine::Transform>(),
        TechEngine::componentTypeId<TechEngine::Name>(),
    };
    for (const ComponentTypeId type: builtIns) {
        CHECK_FALSE(scene.removeComponentRaw(entity, type));
        CHECK_FALSE(scene.addComponentRaw(entity, type));
    }

    CHECK(scene.hasComponent<TechEngine::Transform>(entity));
    CHECK(scene.getComponent<TechEngine::Name>(entity).value == "Kept");
    CHECK(scene.getParent(entity) == parent);
}
