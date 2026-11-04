#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

static_assert(TechEngineTests::WritableSceneComponent<TechEngine::Name>);
static_assert(!TechEngineTests::AddableSceneComponent<TechEngine::Name>);
static_assert(!TechEngineTests::AddableToPendingSceneComponent<TechEngine::Name>);
static_assert(!TechEngineTests::RemovableSceneComponent<TechEngine::Name>);
static_assert(!TechEngineTests::RemovableFromPendingSceneComponent<TechEngine::Name>);

TEST_CASE("every new entity carries a Name called Entity", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);

    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();

    REQUIRE(scene.hasComponent<TechEngine::Name>(first));
    REQUIRE(scene.hasComponent<TechEngine::Name>(second));
    CHECK(scene.getComponent<TechEngine::Name>(first).value == "Entity");
    CHECK(scene.getComponent<TechEngine::Name>(second).value == "Entity");
}

TEST_CASE("renaming one entity leaves the others' names alone", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity renamed = scene.createEntity();
    const TechEngine::Entity untouched = scene.createEntity();

    scene.getComponent<TechEngine::Name>(renamed).value = "Crate";

    CHECK(scene.getComponent<TechEngine::Name>(renamed).value == "Crate");
    CHECK(scene.getComponent<TechEngine::Name>(untouched).value == "Entity");
}

TEST_CASE("an entity created in a slot that held a renamed entity starts with the default name", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity destroyed = scene.createEntity();
    scene.getComponent<TechEngine::Name>(destroyed).value = "Crate";
    REQUIRE(scene.destroyEntity(destroyed));

    const TechEngine::Entity recycled = scene.createEntity();

    REQUIRE(recycled.index == destroyed.index);
    CHECK(scene.getComponent<TechEngine::Name>(recycled).value == "Entity");
}
