#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <typeindex>

struct SchedulePosition {
    float x;
};

struct ScheduleVelocity {
    float x;
};

class MovementSystem final : public TechEngine::ISystem {
public:
    void update(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "MovementSystem";
    }
};

class CollisionSystem final : public TechEngine::ISystem {
public:
    void update(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "CollisionSystem";
    }
};

class ScriptSystem final : public TechEngine::ISystem {
public:
    void update(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "ScriptSystem";
    }
};

TEST_CASE("schedule registration stores a factory and lowers declared access", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId position = registry.registerComponent<SchedulePosition>("Test.SchedulePosition");
    const TechEngine::ComponentTypeId velocity = registry.registerComponent<ScheduleVelocity>("Test.ScheduleVelocity");
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<SchedulePosition>, TechEngine::Read<ScheduleVelocity>>{});

    REQUIRE(schedule.entries().size() == 1);
    const TechEngine::ScheduleEntry& entry = schedule.entries().front();
    REQUIRE(entry.systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(entry.factory != nullptr);
    REQUIRE(entry.factory()->name() == "MovementSystem");
    REQUIRE(entry.access.writes(registry.find(position)->denseId));
    REQUIRE(entry.access.reads(registry.find(position)->denseId));
    REQUIRE(entry.access.reads(registry.find(velocity)->denseId));
    REQUIRE_FALSE(entry.access.writes(registry.find(velocity)->denseId));
}

TEST_CASE("schedule entry metadata is assigned through the registration handle", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>().priority(10).before<CollisionSystem>();
    schedule.add<CollisionSystem>().priority(20).after<MovementSystem>();
    schedule.add<ScriptSystem>().slot(TechEngine::Slot::Terminal);

    REQUIRE(schedule.entries().size() == 3);
    REQUIRE(schedule.entries()[0].priority == 10);
    REQUIRE(schedule.entries()[0].orderConstraints.size() == 1);
    REQUIRE(schedule.entries()[0].orderConstraints.front().systemType == std::type_index(typeid(CollisionSystem)));
    REQUIRE(schedule.entries()[0].orderConstraints.front().order == TechEngine::Order::Before);
    REQUIRE(schedule.entries()[1].priority == 20);
    REQUIRE(schedule.entries()[1].orderConstraints.size() == 1);
    REQUIRE(schedule.entries()[1].orderConstraints.front().systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.entries()[1].orderConstraints.front().order == TechEngine::Order::After);
    REQUIRE(schedule.entries()[2].slot == TechEngine::Slot::Terminal);
}

TEST_CASE("a duplicate system is rejected without changing the schedule", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>();
    REQUIRE_THROWS_AS(schedule.add<MovementSystem>(), TechEngineTests::AssertFired);

    REQUIRE(schedule.entries().size() == 1);
}

TEST_CASE("a frozen schedule rejects registration and entry mutation", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    TechEngine::ScheduleRegistration movement = schedule.add<MovementSystem>();
    schedule.freeze();

    REQUIRE_THROWS_AS(schedule.add<CollisionSystem>(), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.priority(10), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.before<CollisionSystem>(), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.slot(TechEngine::Slot::Terminal), TechEngineTests::AssertFired);

    REQUIRE(schedule.frozen());
    REQUIRE(schedule.entries().size() == 1);
    REQUIRE(schedule.entries().front().priority == 0);
    REQUIRE(schedule.entries().front().orderConstraints.empty());
    REQUIRE(schedule.entries().front().slot == TechEngine::Slot::Regular);
}

TEST_CASE("only one terminal entry can be declared", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<ScriptSystem>().slot(TechEngine::Slot::Terminal);
    TechEngine::ScheduleRegistration collision = schedule.add<CollisionSystem>();
    REQUIRE_THROWS_AS(collision.slot(TechEngine::Slot::Terminal), TechEngineTests::AssertFired);

    REQUIRE(schedule.entries()[0].slot == TechEngine::Slot::Terminal);
    REQUIRE(schedule.entries()[1].slot == TechEngine::Slot::Regular);
}
