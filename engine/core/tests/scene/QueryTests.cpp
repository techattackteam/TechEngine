#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/ArchetypeStorage.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <barrier>
#include <cstddef>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>

struct QueryPosition {
    int value = 0;
};

struct QueryVelocity {
    int value = 0;
};

struct QueryHealth {
    int value = 0;
};

template<typename Component>
concept WritableQueryComponent = requires { sizeof(TechEngine::Query<TechEngine::Write<Component>, TechEngine::Read<>>); };

template<typename Component>
concept ReadableQueryComponent = requires { sizeof(TechEngine::Query<TechEngine::Write<>, TechEngine::Read<Component>>); };

static_assert(!std::is_move_constructible_v<TechEngine::ArchetypeStorage>);
static_assert(!std::is_move_assignable_v<TechEngine::ArchetypeStorage>);
static_assert(!WritableQueryComponent<TechEngine::Hierarchy>);
static_assert(ReadableQueryComponent<TechEngine::Hierarchy>);

TEST_CASE("eachEntity visits every live entity across archetypes", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    registry.registerComponent<QueryVelocity>("Tests.QueryVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity removed = storage.createEntity();
    const TechEngine::Entity empty = storage.createEntity();
    const TechEngine::Entity positioned = storage.createEntity();
    const TechEngine::Entity moving = storage.createEntity();
    REQUIRE(storage.destroyEntity(removed));
    REQUIRE(storage.addComponent(positioned, QueryPosition{1}));
    REQUIRE(storage.addComponent(moving, QueryVelocity{2}));
    std::unordered_set<TechEngine::Entity> visited;

    storage.eachEntity([&](const TechEngine::Entity entity) {
        visited.insert(entity);
    });

    REQUIRE(visited.size() == 3);
    REQUIRE(visited.contains(empty));
    REQUIRE(visited.contains(positioned));
    REQUIRE(visited.contains(moving));
    REQUIRE_FALSE(visited.contains(removed));
}

TEST_CASE("queries iterate every matching archetype with typed access", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    registry.registerComponent<QueryVelocity>("Tests.QueryVelocity");
    registry.registerComponent<QueryHealth>("Tests.QueryHealth");
    TechEngine::ArchetypeStorage storage(registry);

    const TechEngine::Entity positionOnly = storage.createEntity();
    const TechEngine::Entity moving = storage.createEntity();
    const TechEngine::Entity healthy = storage.createEntity();
    REQUIRE(storage.addComponent(positionOnly, QueryPosition{1}));
    REQUIRE(storage.addComponent(moving, QueryPosition{2}));
    REQUIRE(storage.addComponent(moving, QueryVelocity{20}));
    REQUIRE(storage.addComponent(healthy, QueryPosition{3}));
    REQUIRE(storage.addComponent(healthy, QueryHealth{30}));

    auto positions = storage.query<TechEngine::Write<QueryPosition>, TechEngine::Read<>>();
    std::unordered_set<TechEngine::Entity> visited;
    positions.each([&](const TechEngine::Entity entity, QueryPosition& position) {
        visited.insert(entity);
        position.value += 10;
    });

    REQUIRE(visited.size() == 3);
    REQUIRE(visited.contains(positionOnly));
    REQUIRE(visited.contains(moving));
    REQUIRE(visited.contains(healthy));
    REQUIRE(storage.component<QueryPosition>(positionOnly)->value == 11);
    REQUIRE(storage.component<QueryPosition>(moving)->value == 12);
    REQUIRE(storage.component<QueryPosition>(healthy)->value == 13);

    auto movingPositions = storage.query<TechEngine::Write<QueryPosition>, TechEngine::Read<QueryVelocity>>();
    std::size_t movingCount = 0;
    movingPositions.each([&](const TechEngine::Entity entity, QueryPosition& position, const QueryVelocity& velocity) {
        REQUIRE(entity == moving);
        position.value += velocity.value;
        movingCount++;
    });

    REQUIRE(movingCount == 1);
    REQUIRE(storage.component<QueryPosition>(moving)->value == 32);
}

TEST_CASE("scene exposes typed queries without exposing archetype storage", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
    registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    registry.registerComponent<QueryVelocity>("Tests.QueryVelocity");
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    scene.addComponent<QueryPosition>(entity, QueryPosition{2});
    scene.addComponent<QueryVelocity>(entity, QueryVelocity{3});
    auto query = scene.query<TechEngine::Write<QueryPosition>, TechEngine::Read<QueryVelocity>>();

    query.each([](TechEngine::Entity, QueryPosition& position, const QueryVelocity& velocity) {
        position.value += velocity.value;
    });

    REQUIRE(std::as_const(scene).getComponent<QueryPosition>(entity).value == 5);
}

TEST_CASE("queries refresh matches when a new archetype appears", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    TechEngine::ArchetypeStorage storage(registry);
    auto query = storage.query<TechEngine::Write<>, TechEngine::Read<QueryPosition>>();
    std::size_t count = 0;

    query.each([&](TechEngine::Entity, const QueryPosition&) {
        count++;
    });
    REQUIRE(count == 0);

    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, QueryPosition{7}));
    query.each([&](const TechEngine::Entity found, const QueryPosition& position) {
        REQUIRE(found == entity);
        REQUIRE(position.value == 7);
        count++;
    });

    REQUIRE(count == 1);
}

TEST_CASE("queries reacquire spans when an existing archetype grows", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    REQUIRE(storage.addComponent(first, QueryPosition{1}));
    auto query = storage.query<TechEngine::Write<>, TechEngine::Read<QueryPosition>>();
    std::size_t count = 0;

    query.each([&](TechEngine::Entity, const QueryPosition&) {
        count++;
    });
    REQUIRE(count == 1);

    const TechEngine::Entity second = storage.createEntity();
    REQUIRE(storage.addComponent(second, QueryPosition{2}));
    count = 0;
    query.each([&](TechEngine::Entity, const QueryPosition&) {
        count++;
    });

    REQUIRE(count == 2);
}

TEST_CASE("clearing storage invalidates retained query matches", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity stale = storage.createEntity();
    REQUIRE(storage.addComponent(stale, QueryPosition{1}));
    auto query = storage.query<TechEngine::Write<>, TechEngine::Read<QueryPosition>>();
    std::size_t count = 0;
    query.each([&](TechEngine::Entity, const QueryPosition&) {
        count++;
    });
    REQUIRE(count == 1);

    storage.clear();
    count = 0;
    query.each([&](TechEngine::Entity, const QueryPosition&) {
        count++;
    });
    REQUIRE(count == 0);

    const TechEngine::Entity current = storage.createEntity();
    REQUIRE(storage.addComponent(current, QueryPosition{2}));
    query.each([&](const TechEngine::Entity entity, const QueryPosition& position) {
        REQUIRE(entity == current);
        REQUIRE(position.value == 2);
        count++;
    });
    REQUIRE(count == 1);
}

TEST_CASE("structural mutation is rejected during query iteration", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    registry.registerComponent<QueryVelocity>("Tests.QueryVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, QueryPosition{1}));
    auto query = storage.query<TechEngine::Write<QueryPosition>, TechEngine::Read<>>();

    SECTION("create") {
        REQUIRE_THROWS_AS(
            query.each([&](TechEngine::Entity, QueryPosition&) {
                storage.createEntity();
            }),
            TechEngineTests::AssertFired);
    }

    SECTION("destroy") {
        REQUIRE_THROWS_AS(
            query.each([&](TechEngine::Entity, QueryPosition&) {
                storage.destroyEntity(entity);
            }),
            TechEngineTests::AssertFired);
    }

    SECTION("add") {
        REQUIRE_THROWS_AS(
            query.each([&](TechEngine::Entity, QueryPosition&) {
                storage.addComponent(entity, QueryVelocity{2});
            }),
            TechEngineTests::AssertFired);
    }

    SECTION("remove") {
        REQUIRE_THROWS_AS(
            query.each([&](TechEngine::Entity, QueryPosition&) {
                storage.removeComponent<QueryPosition>(entity);
            }),
            TechEngineTests::AssertFired);
    }

    SECTION("clear") {
        REQUIRE_THROWS_AS(
            query.each([&](TechEngine::Entity, QueryPosition&) {
                storage.clear();
            }),
            TechEngineTests::AssertFired);
    }

    SECTION("each entity") {
        REQUIRE_THROWS_AS(
            storage.eachEntity([&](TechEngine::Entity) {
                storage.destroyEntity(entity);
            }),
            TechEngineTests::AssertFired);
    }

    REQUIRE(storage.removeComponent<QueryPosition>(entity));
}

TEST_CASE("query iteration state unwinds when a callback throws", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, QueryPosition{1}));
    auto query = storage.query<TechEngine::Write<QueryPosition>, TechEngine::Read<>>();

    REQUIRE_THROWS_AS(
        query.each([](TechEngine::Entity, QueryPosition&) {
            throw std::runtime_error("query callback failed");
        }),
        std::runtime_error);

    REQUIRE(storage.removeComponent<QueryPosition>(entity));
}

TEST_CASE("disjoint queries can iterate concurrently", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<QueryPosition>("Tests.QueryPosition");
    registry.registerComponent<QueryVelocity>("Tests.QueryVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity positioned = storage.createEntity();
    const TechEngine::Entity moving = storage.createEntity();
    REQUIRE(storage.addComponent(positioned, QueryPosition{11}));
    REQUIRE(storage.addComponent(moving, QueryVelocity{20}));

    auto positions = storage.query<TechEngine::Write<>, TechEngine::Read<QueryPosition>>();
    auto velocities = storage.query<TechEngine::Write<QueryVelocity>, TechEngine::Read<>>();
    std::barrier<> iterationBarrier(2);
    std::atomic<int> observedPosition = 0;

    std::jthread positionThread([&] {
        positions.each([&](TechEngine::Entity, const QueryPosition& position) {
            iterationBarrier.arrive_and_wait();
            observedPosition.store(position.value, std::memory_order_relaxed);
        });
    });
    std::jthread velocityThread([&] {
        velocities.each([&](TechEngine::Entity, QueryVelocity& velocity) {
            iterationBarrier.arrive_and_wait();
            velocity.value++;
        });
    });
    positionThread.join();
    velocityThread.join();

    REQUIRE(observedPosition.load(std::memory_order_relaxed) == 11);
    REQUIRE(storage.component<QueryVelocity>(moving)->value == 21);
}
