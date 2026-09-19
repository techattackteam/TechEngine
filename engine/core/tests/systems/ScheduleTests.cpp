#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>

struct SchedulePosition {
    float x;
};

struct ScheduleVelocity {
    float x;
};

struct ScheduleUnregistered {
    float value;
};

template<std::size_t Index>
struct ScheduleMaskComponent {
    std::uint32_t value;
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

template<std::size_t Index>
class SchedulePlaceholderSystem final : public TechEngine::ISystem {
public:
    void update(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "SchedulePlaceholderSystem";
    }
};

template<std::size_t... Indices>
static void registerMaskComponents(TechEngine::ComponentRegistry& registry, std::index_sequence<Indices...>) {
    (registry.registerComponent<ScheduleMaskComponent<Indices>>("Test.ScheduleMask." + std::to_string(Indices)), ...);
}

template<std::size_t... Indices>
static void addPlaceholderSystems(TechEngine::Schedule& schedule, std::index_sequence<Indices...>) {
    (schedule.add<SchedulePlaceholderSystem<Indices>>(), ...);
}

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

TEST_CASE("schedule access crosses mask word boundaries without touching neighboring types", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    registerMaskComponents(registry, std::make_index_sequence<65>{});
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<ScheduleMaskComponent<0>, ScheduleMaskComponent<64>>, TechEngine::Read<ScheduleMaskComponent<1>, ScheduleMaskComponent<63>>>{});

    const TechEngine::ScheduleAccess& access = schedule.entries().front().access;
    const TechEngine::ComponentDenseId zero = registry.find(TechEngine::componentTypeId<ScheduleMaskComponent<0>>())->denseId;
    const TechEngine::ComponentDenseId one = registry.find(TechEngine::componentTypeId<ScheduleMaskComponent<1>>())->denseId;
    const TechEngine::ComponentDenseId sixtyTwo = registry.find(TechEngine::componentTypeId<ScheduleMaskComponent<62>>())->denseId;
    const TechEngine::ComponentDenseId sixtyThree = registry.find(TechEngine::componentTypeId<ScheduleMaskComponent<63>>())->denseId;
    const TechEngine::ComponentDenseId sixtyFour = registry.find(TechEngine::componentTypeId<ScheduleMaskComponent<64>>())->denseId;

    REQUIRE(access.writes(zero));
    REQUIRE(access.reads(zero));
    REQUIRE(access.touches(zero));
    REQUIRE(access.reads(one));
    REQUIRE_FALSE(access.writes(one));
    REQUIRE(access.touches(one));
    REQUIRE_FALSE(access.reads(sixtyTwo));
    REQUIRE_FALSE(access.writes(sixtyTwo));
    REQUIRE_FALSE(access.touches(sixtyTwo));
    REQUIRE(access.reads(sixtyThree));
    REQUIRE_FALSE(access.writes(sixtyThree));
    REQUIRE(access.touches(sixtyThree));
    REQUIRE(access.writes(sixtyFour));
    REQUIRE(access.reads(sixtyFour));
    REQUIRE(access.touches(sixtyFour));
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

TEST_CASE("a retained registration handle survives later schedule growth", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    TechEngine::ScheduleRegistration movement = schedule.add<MovementSystem>();

    addPlaceholderSystems(schedule, std::make_index_sequence<32>{});
    movement.priority(17).before<CollisionSystem>();

    REQUIRE(schedule.entries().size() == 33);
    REQUIRE(schedule.entries().front().systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.entries().front().priority == 17);
    REQUIRE(schedule.entries().front().orderConstraints.size() == 1);
    REQUIRE(schedule.entries().front().orderConstraints.front().systemType == std::type_index(typeid(CollisionSystem)));
    REQUIRE(schedule.entries().front().orderConstraints.front().order == TechEngine::Order::Before);
}

TEST_CASE("a duplicate system is rejected without changing the schedule", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>();
    REQUIRE_THROWS_AS(schedule.add<MovementSystem>(), TechEngineTests::AssertFired);

    schedule.add<CollisionSystem>();

    REQUIRE(schedule.entries().size() == 2);
    REQUIRE(schedule.entries()[0].systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.entries()[1].systemType == std::type_index(typeid(CollisionSystem)));
}

TEST_CASE("unregistered access is rejected without publishing a partial entry", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<SchedulePosition>("Test.SchedulePosition");
    TechEngine::Schedule schedule(registry);

    REQUIRE_THROWS_AS(schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<ScheduleUnregistered>, TechEngine::Read<>>{}), TechEngineTests::AssertFired);
    REQUIRE(schedule.entries().empty());

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<SchedulePosition>, TechEngine::Read<>>{});
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

    TechEngine::ScheduleRegistration script = schedule.add<ScriptSystem>();
    script.slot(TechEngine::Slot::Terminal);
    TechEngine::ScheduleRegistration collision = schedule.add<CollisionSystem>();
    REQUIRE_THROWS_AS(collision.slot(TechEngine::Slot::Terminal), TechEngineTests::AssertFired);

    REQUIRE(schedule.entries()[0].slot == TechEngine::Slot::Terminal);
    REQUIRE(schedule.entries()[1].slot == TechEngine::Slot::Regular);

    script.slot(TechEngine::Slot::Terminal);
    script.slot(TechEngine::Slot::Regular);
    collision.slot(TechEngine::Slot::Terminal);

    REQUIRE(schedule.entries()[0].slot == TechEngine::Slot::Regular);
    REQUIRE(schedule.entries()[1].slot == TechEngine::Slot::Terminal);
}
