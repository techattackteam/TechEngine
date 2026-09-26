#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

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
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "MovementSystem";
    }
};

class CollisionSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "CollisionSystem";
    }
};

class ScriptSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "ScriptSystem";
    }
};

class DescribingSystem final : public TechEngine::ISystem {
public:
    static inline int constructions = 0;
    static inline int startups = 0;

    DescribingSystem() {
        constructions++;
    }

    void init(TechEngine::ScheduleRegistration& registration) override {
        startups++;
        registration.access(TechEngine::DeclareAccess<TechEngine::Write<SchedulePosition>, TechEngine::Read<>>{});
        registration.access(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<ScheduleVelocity>>{});
        registration.setPriority(13);
        registration.before<MovementSystem>();
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "DescribingSystem";
    }
};

class FailingDescriptionSystem final : public TechEngine::ISystem {
public:
    static inline int destructions = 0;

    ~FailingDescriptionSystem() override {
        destructions++;
    }

    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.setPriority(17);
        throw std::runtime_error("startup declaration failed");
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "FailingDescriptionSystem";
    }
};

template<typename System>
concept CanScheduleHierarchyWrite = requires(TechEngine::Schedule& schedule) { schedule.add<System>(TechEngine::DeclareAccess<TechEngine::Write<TechEngine::Hierarchy>, TechEngine::Read<>>{}); };

static_assert(!CanScheduleHierarchyWrite<MovementSystem>);

template<std::size_t Index>
class SchedulePlaceholderSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
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

TEST_CASE("schedule registration retains the selected instance and lowers declared access", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId position = registry.registerComponent<SchedulePosition>("Test.SchedulePosition");
    const TechEngine::ComponentTypeId velocity = registry.registerComponent<ScheduleVelocity>("Test.ScheduleVelocity");
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<SchedulePosition>, TechEngine::Read<ScheduleVelocity>>{});

    REQUIRE(schedule.getEntries().size() == 1);
    const TechEngine::ScheduleEntry& entry = schedule.getEntries().front();
    REQUIRE(entry.systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(entry.system != nullptr);
    REQUIRE(entry.name == "MovementSystem");
    REQUIRE(entry.access.writes(registry.find(position)->denseId));
    REQUIRE(entry.access.reads(registry.find(position)->denseId));
    REQUIRE(entry.access.reads(registry.find(velocity)->denseId));
    REQUIRE_FALSE(entry.access.writes(registry.find(velocity)->denseId));
    std::vector<TechEngine::ComponentDenseId> writtenTypes;
    std::vector<TechEngine::ComponentDenseId> readTypes;
    entry.access.forEachWrittenType([&writtenTypes](const TechEngine::ComponentDenseId type) {
        writtenTypes.push_back(type);
    });
    entry.access.forEachReadType([&readTypes](const TechEngine::ComponentDenseId type) {
        readTypes.push_back(type);
    });
    REQUIRE(writtenTypes == std::vector{registry.find(position)->denseId});
    REQUIRE(readTypes == std::vector{registry.find(velocity)->denseId});
}

TEST_CASE("a selected system describes its own entry once at registration", "[core][systems]") {
    DescribingSystem::constructions = 0;
    DescribingSystem::startups = 0;
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId position = registry.registerComponent<SchedulePosition>("Test.SchedulePosition");
    const TechEngine::ComponentTypeId velocity = registry.registerComponent<ScheduleVelocity>("Test.ScheduleVelocity");
    TechEngine::Schedule schedule(registry);

    schedule.add<DescribingSystem>();

    REQUIRE(DescribingSystem::constructions == 1);
    REQUIRE(DescribingSystem::startups == 1);
    REQUIRE(schedule.getEntries().size() == 1);
    const TechEngine::ScheduleEntry& entry = schedule.getEntries().front();
    REQUIRE(entry.name == "DescribingSystem");
    REQUIRE(entry.priority == 13);
    REQUIRE(entry.access.writes(registry.find(position)->denseId));
    REQUIRE(entry.access.reads(registry.find(velocity)->denseId));
    const std::vector expectedOrder{TechEngine::OrderConstraint{typeid(MovementSystem), TechEngine::Order::Before}};
    REQUIRE(entry.orderConstraints == expectedOrder);
}

TEST_CASE("a failed startup declaration removes its entry and destroys its instance", "[core][systems]") {
    FailingDescriptionSystem::destructions = 0;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    REQUIRE_THROWS_AS(schedule.add<FailingDescriptionSystem>(), std::runtime_error);

    REQUIRE(schedule.getEntries().empty());
    REQUIRE(FailingDescriptionSystem::destructions == 1);
    schedule.add<MovementSystem>();
    REQUIRE(schedule.getEntries().size() == 1);
}

TEST_CASE("schedule access crosses mask word boundaries without touching neighboring types", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    registerMaskComponents(registry, std::make_index_sequence<65>{});
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<ScheduleMaskComponent<64>, ScheduleMaskComponent<0>>, TechEngine::Read<ScheduleMaskComponent<63>, ScheduleMaskComponent<1>>>{});

    const TechEngine::ScheduleAccess& access = schedule.getEntries().front().access;
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

    std::vector<TechEngine::ComponentDenseId> writtenTypes;
    std::vector<TechEngine::ComponentDenseId> readTypes;
    access.forEachWrittenType([&writtenTypes](const TechEngine::ComponentDenseId type) {
        writtenTypes.push_back(type);
    });
    access.forEachReadType([&readTypes](const TechEngine::ComponentDenseId type) {
        readTypes.push_back(type);
    });
    const std::vector expectedWrittenTypes{zero, sixtyFour};
    const std::vector expectedReadTypes{one, sixtyThree};
    REQUIRE(writtenTypes == expectedWrittenTypes);
    REQUIRE(readTypes == expectedReadTypes);

    const TechEngine::ScheduleAccess empty;
    std::size_t emptyTypeCount = 0;
    empty.forEachWrittenType([&emptyTypeCount](TechEngine::ComponentDenseId) {
        emptyTypeCount++;
    });
    empty.forEachReadType([&emptyTypeCount](TechEngine::ComponentDenseId) {
        emptyTypeCount++;
    });
    REQUIRE(emptyTypeCount == 0);
}

TEST_CASE("schedule entry metadata is assigned through the registration handle", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>().setPriority(10).before<CollisionSystem>();
    schedule.add<CollisionSystem>().setPriority(20).after<MovementSystem>();
    schedule.add<ScriptSystem>().setSlot(TechEngine::Slot::Terminal);

    REQUIRE(schedule.getEntries().size() == 3);
    REQUIRE(schedule.getEntries()[0].priority == 10);
    REQUIRE(schedule.getEntries()[0].orderConstraints.size() == 1);
    REQUIRE(schedule.getEntries()[0].orderConstraints.front().systemType == std::type_index(typeid(CollisionSystem)));
    REQUIRE(schedule.getEntries()[0].orderConstraints.front().order == TechEngine::Order::Before);
    REQUIRE(schedule.getEntries()[1].priority == 20);
    REQUIRE(schedule.getEntries()[1].orderConstraints.size() == 1);
    REQUIRE(schedule.getEntries()[1].orderConstraints.front().systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.getEntries()[1].orderConstraints.front().order == TechEngine::Order::After);
    REQUIRE(schedule.getEntries()[2].slot == TechEngine::Slot::Terminal);
}

TEST_CASE("a retained registration handle survives later schedule growth", "[core][systems]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    TechEngine::ScheduleRegistration movement = schedule.add<MovementSystem>();

    addPlaceholderSystems(schedule, std::make_index_sequence<32>{});
    movement.setPriority(17).before<CollisionSystem>();

    REQUIRE(schedule.getEntries().size() == 33);
    REQUIRE(schedule.getEntries().front().systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.getEntries().front().priority == 17);
    REQUIRE(schedule.getEntries().front().orderConstraints.size() == 1);
    REQUIRE(schedule.getEntries().front().orderConstraints.front().systemType == std::type_index(typeid(CollisionSystem)));
    REQUIRE(schedule.getEntries().front().orderConstraints.front().order == TechEngine::Order::Before);
}

TEST_CASE("a duplicate system is rejected without changing the schedule", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    schedule.add<MovementSystem>();
    REQUIRE_THROWS_AS(schedule.add<MovementSystem>(), TechEngineTests::AssertFired);

    schedule.add<CollisionSystem>();

    REQUIRE(schedule.getEntries().size() == 2);
    REQUIRE(schedule.getEntries()[0].systemType == std::type_index(typeid(MovementSystem)));
    REQUIRE(schedule.getEntries()[1].systemType == std::type_index(typeid(CollisionSystem)));
}

TEST_CASE("unregistered access is rejected without publishing a partial entry", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<SchedulePosition>("Test.SchedulePosition");
    TechEngine::Schedule schedule(registry);

    REQUIRE_THROWS_AS(schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<ScheduleUnregistered>, TechEngine::Read<>>{}), TechEngineTests::AssertFired);
    REQUIRE(schedule.getEntries().empty());

    schedule.add<MovementSystem>(TechEngine::DeclareAccess<TechEngine::Write<SchedulePosition>, TechEngine::Read<>>{});
    REQUIRE(schedule.getEntries().size() == 1);
}

TEST_CASE("a frozen schedule rejects registration and entry mutation", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    TechEngine::ScheduleRegistration movement = schedule.add<MovementSystem>();
    schedule.freeze();

    REQUIRE_THROWS_AS(schedule.add<CollisionSystem>(), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.setPriority(10), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.before<CollisionSystem>(), TechEngineTests::AssertFired);
    REQUIRE_THROWS_AS(movement.setSlot(TechEngine::Slot::Terminal), TechEngineTests::AssertFired);

    REQUIRE(schedule.frozen());
    REQUIRE(schedule.getEntries().size() == 1);
    REQUIRE(schedule.getEntries().front().priority == 0);
    REQUIRE(schedule.getEntries().front().orderConstraints.empty());
    REQUIRE(schedule.getEntries().front().slot == TechEngine::Slot::Regular);
}

TEST_CASE("only one terminal entry can be declared", "[core][systems]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    TechEngine::ScheduleRegistration script = schedule.add<ScriptSystem>();
    script.setSlot(TechEngine::Slot::Terminal);
    TechEngine::ScheduleRegistration collision = schedule.add<CollisionSystem>();
    REQUIRE_THROWS_AS(collision.setSlot(TechEngine::Slot::Terminal), TechEngineTests::AssertFired);

    REQUIRE(schedule.getEntries()[0].slot == TechEngine::Slot::Terminal);
    REQUIRE(schedule.getEntries()[1].slot == TechEngine::Slot::Regular);

    script.setSlot(TechEngine::Slot::Terminal);
    script.setSlot(TechEngine::Slot::Regular);
    collision.setSlot(TechEngine::Slot::Terminal);

    REQUIRE(schedule.getEntries()[0].slot == TechEngine::Slot::Regular);
    REQUIRE(schedule.getEntries()[1].slot == TechEngine::Slot::Terminal);
}
