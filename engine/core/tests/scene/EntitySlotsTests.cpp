#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/EntitySlots.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the default entity is null", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    const TechEngine::Entity entity;

    REQUIRE_FALSE(entity.valid());
    REQUIRE(entity.index == TechEngine::Entity::NULL_INDEX);
}

TEST_CASE("creating an entity returns a live handle", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots;

    const TechEngine::Entity entity = slots.create();

    REQUIRE(entity.valid());
    REQUIRE(slots.contains(entity));
    REQUIRE(slots.size() == 1);
}

TEST_CASE("destroying an entity invalidates its handle", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots;
    const TechEngine::Entity entity = slots.create();

    REQUIRE(slots.destroy(entity));

    REQUIRE_FALSE(slots.contains(entity));
    REQUIRE(slots.size() == 0);
}

TEST_CASE("a destroyed slot is reused with an advanced generation", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots;
    const TechEngine::Entity first = slots.create();
    REQUIRE(slots.destroy(first));

    const TechEngine::Entity reused = slots.create();

    REQUIRE(reused.index == first.index);
    REQUIRE(reused.generation > first.generation);
    REQUIRE_FALSE(slots.contains(first));
    REQUIRE(slots.contains(reused));
}

TEST_CASE("null and stale handles cannot be destroyed", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots;
    const TechEngine::Entity entity = slots.create();
    REQUIRE(slots.destroy(entity));

    REQUIRE_FALSE(slots.destroy({}));
    REQUIRE_FALSE(slots.destroy(entity));
}

TEST_CASE("clear invalidates handles without restarting their generations", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots;
    const TechEngine::Entity beforeClear = slots.create();

    slots.clear();
    const TechEngine::Entity afterClear = slots.create();

    REQUIRE_FALSE(slots.contains(beforeClear));
    REQUIRE(afterClear.index == beforeClear.index);
    REQUIRE(afterClear.generation > beforeClear.generation);
}

TEST_CASE("a generation that would wrap retires its slot", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots{1, 1};
    const TechEngine::Entity first = slots.create();
    REQUIRE(slots.destroy(first));
    const TechEngine::Entity lastGeneration = slots.create();
    REQUIRE(slots.destroy(lastGeneration));

    REQUIRE_THROWS_AS(slots.create(), TechEngineTests::AssertFired);
    REQUIRE(slots.size() == 0);
}

TEST_CASE("slot exhaustion is fatal", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::EntitySlots slots(1);
    REQUIRE(slots.create().valid());

    REQUIRE_THROWS_AS(slots.create(), TechEngineTests::AssertFired);
    REQUIRE(slots.size() == 1);
}
