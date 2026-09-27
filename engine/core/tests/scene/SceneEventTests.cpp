#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

struct SceneEventHit {
    std::uint32_t amount;
};

struct SceneEventHeal {
    std::uint32_t amount;
};

struct SceneEventLate {
    std::uint32_t amount;
};

using Amounts = std::vector<std::uint32_t>;

struct SceneEventTestState {
    std::optional<std::uint32_t> publish;
    std::optional<std::uint32_t> publishHeal;
    bool publishLate = false;
    bool makeVisibleInsideSystem = false;
    bool retireInsideSystem = false;
    std::size_t republishFromHandler = 0;
    bool failAfterDespawn = false;
    TechEngine::Entity target;
    std::vector<Amounts> reads;
    std::vector<Amounts> terminalReads;
    std::vector<std::string_view> trace;
};

static SceneEventTestState* g_sceneEventState = nullptr;

class SceneEventStateGuard {
public:
    explicit SceneEventStateGuard(SceneEventTestState& state) {
        REQUIRE(g_sceneEventState == nullptr);
        g_sceneEventState = &state;
    }

    ~SceneEventStateGuard() {
        g_sceneEventState = nullptr;
    }

    SceneEventStateGuard(const SceneEventStateGuard&) = delete;

    SceneEventStateGuard& operator=(const SceneEventStateGuard&) = delete;
};

template<typename Event>
static Amounts amountsOf(const std::span<const Event> events) {
    Amounts amounts;
    for (const Event& event: events) {
        amounts.push_back(event.amount);
    }
    return amounts;
}

class SceneEventPublisherSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext& context) override {
        if (g_sceneEventState->publish) {
            scene.publish(SceneEventHit{*g_sceneEventState->publish});
        }
        if (g_sceneEventState->publishHeal) {
            scene.publish(SceneEventHeal{*g_sceneEventState->publishHeal});
        }
        if (g_sceneEventState->publishLate) {
            scene.publish(SceneEventLate{1});
        }
        if (g_sceneEventState->makeVisibleInsideSystem) {
            scene.makeEventsVisible(context.tick);
        }
        if (g_sceneEventState->retireInsideSystem) {
            scene.retireEvents();
        }
    }

    std::string_view name() const override {
        return "SceneEventPublisherSystem";
    }
};

class SceneEventReaderSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.on<SceneEventHeal>([](TechEngine::Scene&, std::span<const SceneEventHeal>) {
            g_sceneEventState->trace.push_back("heal");
        });
        registration.on<SceneEventHit>([](TechEngine::Scene& scene, const std::span<const SceneEventHit> hits) {
            g_sceneEventState->trace.push_back("hit");
            for (std::size_t i = 0; i < g_sceneEventState->republishFromHandler; i++) {
                scene.publish(SceneEventHit{100});
            }
            g_sceneEventState->reads.push_back(amountsOf(hits));
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_sceneEventState->trace.push_back("tick");
    }

    std::string_view name() const override {
        return "SceneEventReaderSystem";
    }
};

class SceneEventFailingSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        if (!g_sceneEventState->failAfterDespawn) {
            return;
        }
        scene.getCommands().despawn(g_sceneEventState->target);
        throw std::runtime_error("system failure");
    }

    std::string_view name() const override {
        return "SceneEventFailingSystem";
    }
};

class SceneEventTerminalReaderSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.setSlot(TechEngine::Slot::Terminal).on<SceneEventHit>([](TechEngine::Scene&, const std::span<const SceneEventHit> hits) {
            g_sceneEventState->terminalReads.push_back(amountsOf(hits));
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "SceneEventTerminalReaderSystem";
    }
};

class SceneEventBarrier final : public TechEngine::TickBarrierServices {
public:
    void assignNetIds(TechEngine::Scene&, std::span<const TechEngine::Entity>) override {
    }
};

class SceneEventFixture {
public:
    TechEngine::ComponentRegistry components;
    TechEngine::EventRegistry events;
    TechEngine::Scene first;
    TechEngine::Scene second;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files;
    TechEngine::JobSystem jobs;
    TechEngine::Clock clock;
    TechEngine::EngineContext engine;
    TechEngine::InputFrame input;
    TechEngine::SimulationContext context;
    TechEngine::Schedule schedule;
    std::optional<TechEngine::TaskGraph> graph;
    std::optional<TechEngine::SerialExecutor> executor;
    SceneEventBarrier barrier;

    SceneEventFixture() : first(components), second(components), files(mounts), jobs(1), engine{files, jobs, clock}, context{.fixedDeltaTime = 1.0 / 60.0, .tick = 1, .input = input, .engine = engine}, schedule(components) {
        TechEngineTests::registerBuiltInSceneComponents(components);
        events.registerEvent<SceneEventHit>("Test.SceneEventHit");
        events.registerEvent<SceneEventHeal>("Test.SceneEventHeal");
        schedule.add<SceneEventPublisherSystem>().before<SceneEventReaderSystem>();
        schedule.add<SceneEventReaderSystem>().before<SceneEventFailingSystem>();
        schedule.add<SceneEventFailingSystem>();
        schedule.add<SceneEventTerminalReaderSystem>();
        graph.emplace(schedule, events);
        executor.emplace(*graph);
    }

    void buildStreams() {
        first.buildEventStreams(events);
        second.buildEventStreams(events);
    }

    void runTick(TechEngine::Scene& scene) {
        executor->execute(scene, context, barrier);
    }
};

TEST_CASE("building a Scene's event streams closes event registration", "[core][scene][events]") {
    SceneEventFixture fixture;

    REQUIRE_FALSE(fixture.events.sealed());

    fixture.first.buildEventStreams(fixture.events);

    REQUIRE(fixture.events.sealed());
    REQUIRE(fixture.events.typeCount() == 2);
}

TEST_CASE("an event reaches every handler in the next Tick, including the terminal slot, then retires", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.empty());
    REQUIRE(state.terminalReads.empty());

    state.publish = 9;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}});

    state.publish.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}, {9}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}, {9}});

    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}, {9}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}, {9}});
}

TEST_CASE("handlers run in declaration order before their system's tick", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    state.publishHeal = 3;
    fixture.runTick(fixture.first);
    state.publish.reset();
    state.publishHeal.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.trace == std::vector<std::string_view>{"tick", "heal", "hit", "tick"});
}

TEST_CASE("a handler whose type has no visible events is not called", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publishHeal = 3;
    fixture.runTick(fixture.first);
    state.publishHeal.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.trace == std::vector<std::string_view>{"tick", "heal", "tick", "tick"});
    REQUIRE(state.reads.empty());
    REQUIRE(state.terminalReads.empty());
}

TEST_CASE("a handler publishing its own type reads a stable batch and delivers next Tick", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();
    state.republishFromHandler = 200;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}});

    state.republishFromHandler = 0;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.size() == 2);
    REQUIRE(state.reads.at(1) == Amounts(200, 100));
    REQUIRE(state.terminalReads.at(1) == Amounts(200, 100));
}

TEST_CASE("a failed system phase keeps the visible batch and discards pending structural commands", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();
    state.target = fixture.first.createEntity();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();

    state.failAfterDespawn = true;
    fixture.context.tick++;
    REQUIRE_THROWS_AS(fixture.runTick(fixture.first), std::runtime_error);

    REQUIRE(fixture.first.contains(state.target));
    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(state.terminalReads.empty());

    state.failAfterDespawn = false;
    fixture.runTick(fixture.first);

    REQUIRE(fixture.first.contains(state.target));
    REQUIRE(state.reads == std::vector<Amounts>{{7}, {7}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}});

    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.size() == 2);
    REQUIRE(state.terminalReads.size() == 1);
}

TEST_CASE("two Scenes sharing an event type do not share its events", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();
    fixture.runTick(fixture.second);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});

    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
}

TEST_CASE("each Scene delivers only the events it published", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish = 9;
    fixture.runTick(fixture.second);
    state.publish.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});

    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{7}, {9}});
}

TEST_CASE("one Scene's Tick barrier leaves the other Scene's events alone", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish = 9;
    fixture.runTick(fixture.second);
    state.publish.reset();

    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});

    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{7}, {9}});
}

TEST_CASE("publishing outside a system is rejected and stages nothing", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    fixture.first.publish(SceneEventHit{5});

    REQUIRE(TechEngineTests::g_fired.size() == 1);

    fixture.runTick(fixture.first);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.empty());
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("a system cannot make events visible during its own Tick", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    state.makeVisibleInsideSystem = true;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.empty());
    REQUIRE(TechEngineTests::g_fired.size() == 1);

    state.publish.reset();
    state.makeVisibleInsideSystem = false;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("a system cannot retire the visible batch during a Tick", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();

    state.retireInsideSystem = true;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(state.terminalReads == std::vector<Amounts>{{7}});
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("a late event registration is rejected without corrupting either Scene", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    const TechEngine::EventTypeId late = fixture.events.registerEvent<SceneEventLate>("Test.SceneEventLate");

    REQUIRE_FALSE(late.valid());
    REQUIRE(fixture.events.typeCount() == 2);
    REQUIRE(fixture.events.tagOf(TechEngine::EventTypeId{TechEngine::StringId{"Test.SceneEventLate"}}).empty());

    // The seal rejection is a report-once TE_ENSURE shared with EventRegistryTests, so its count is not asserted here.
    const std::size_t firedAfterRegistration = TechEngineTests::g_fired.size();

    state.publishLate = true;
    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish = 9;
    fixture.runTick(fixture.second);

    REQUIRE(TechEngineTests::g_fired.size() == firedAfterRegistration + 2);

    state.publishLate = false;
    state.publish.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{7}, {9}});
    REQUIRE(TechEngineTests::g_fired.size() == firedAfterRegistration + 2);
}

TEST_CASE("building a Scene's event streams twice is rejected and keeps its staged events", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.first.buildEventStreams(fixture.events);

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();

    fixture.first.buildEventStreams(fixture.events);

    REQUIRE(TechEngineTests::g_fired.size() == 1);

    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("publishing and handling on a Scene without streams are each rejected", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);

    state.publish = 7;
    fixture.runTick(fixture.first);

    REQUIRE(TechEngineTests::g_fired.size() == 4);
    REQUIRE(state.reads.empty());
    REQUIRE(state.terminalReads.empty());
    REQUIRE(state.trace == std::vector<std::string_view>{"tick"});
}

TEST_CASE("the barrier on a Scene without streams does nothing and fires nothing", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);

    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.first.retireEvents();

    REQUIRE(TechEngineTests::g_fired.empty());

    fixture.first.buildEventStreams(fixture.events);
    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{7}});
    REQUIRE(TechEngineTests::g_fired.empty());
}
