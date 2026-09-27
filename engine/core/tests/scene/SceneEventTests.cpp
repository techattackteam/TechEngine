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
#include <string_view>
#include <utility>
#include <vector>

struct SceneEventHit {
    std::uint32_t amount;
};

struct SceneEventLate {
    std::uint32_t amount;
};

struct SceneEventTestState {
    std::optional<std::uint32_t> publish;
    bool publishLate = false;
    bool makeVisibleInsideSystem = false;
    bool retireInsideSystem = false;
    std::vector<std::vector<std::uint32_t>> reads;
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

class SceneEventPublisherSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext& context) override {
        if (g_sceneEventState->publish) {
            scene.publish(SceneEventHit{*g_sceneEventState->publish});
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
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        std::vector<std::uint32_t> amounts;
        for (const SceneEventHit& hit: scene.read<SceneEventHit>()) {
            amounts.push_back(hit.amount);
        }
        g_sceneEventState->reads.push_back(std::move(amounts));
    }

    std::string_view name() const override {
        return "SceneEventReaderSystem";
    }
};

class SceneEventBarrier final : public TechEngine::TickBarrierServices {
public:
    void assignNetIds(TechEngine::Scene&, std::span<const TechEngine::Entity>) override {
    }

    void flushEvents(std::uint64_t) override {
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
        schedule.add<SceneEventPublisherSystem>().before<SceneEventReaderSystem>();
        schedule.add<SceneEventReaderSystem>();
        graph.emplace(schedule);
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

using Amounts = std::vector<std::uint32_t>;

TEST_CASE("building a Scene's event streams closes event registration", "[core][scene][events]") {
    SceneEventFixture fixture;

    REQUIRE_FALSE(fixture.events.sealed());

    fixture.first.buildEventStreams(fixture.events);

    REQUIRE(fixture.events.sealed());
    REQUIRE(fixture.events.typeCount() == 1);
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
    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.second.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{}, {}, {7}, {}});
}

TEST_CASE("each Scene reads back only the events it published", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish = 9;
    fixture.runTick(fixture.second);
    state.publish.reset();
    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.second.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads == std::vector<Amounts>{{}, {}, {7}, {9}});
}

TEST_CASE("the barrier on one Scene leaves the other Scene's events alone", "[core][scene][events]") {
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish = 9;
    fixture.runTick(fixture.second);
    state.publish.reset();

    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads.at(2) == Amounts{7});
    REQUIRE(state.reads.at(3).empty());

    fixture.first.retireEvents();
    fixture.second.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads.at(4).empty());
    REQUIRE(state.reads.at(5) == Amounts{9});
}

TEST_CASE("publishing outside a system is rejected and stages nothing", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    fixture.first.publish(SceneEventHit{5});

    REQUIRE(TechEngineTests::g_fired.size() == 1);

    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads == std::vector<Amounts>{{}});
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("reading outside a system is rejected and leaves the visible batch intact", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);
    fixture.buildStreams();

    state.publish = 7;
    fixture.runTick(fixture.first);
    state.publish.reset();
    fixture.first.makeEventsVisible(fixture.context.tick);

    const std::span<const SceneEventHit> outside = fixture.first.read<SceneEventHit>();

    REQUIRE(outside.empty());
    REQUIRE(TechEngineTests::g_fired.size() == 1);

    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.back() == Amounts{7});
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

    REQUIRE(state.reads == std::vector<Amounts>{{}});
    REQUIRE(TechEngineTests::g_fired.size() == 1);

    state.publish.reset();
    state.makeVisibleInsideSystem = false;
    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.back() == Amounts{7});
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
    fixture.first.makeEventsVisible(fixture.context.tick);

    state.retireInsideSystem = true;
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.back() == Amounts{7});
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
    REQUIRE(fixture.events.typeCount() == 1);
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
    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.second.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);
    fixture.runTick(fixture.second);

    REQUIRE(state.reads.at(2) == Amounts{7});
    REQUIRE(state.reads.at(3) == Amounts{9});
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

    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.back() == Amounts{7});
    REQUIRE(TechEngineTests::g_fired.size() == 1);
}

TEST_CASE("publishing and reading on a Scene without streams are each rejected", "[core][scene][events]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    SceneEventFixture fixture;
    SceneEventTestState state;
    const SceneEventStateGuard stateGuard(state);

    state.publish = 7;
    fixture.runTick(fixture.first);

    REQUIRE(TechEngineTests::g_fired.size() == 2);
    REQUIRE(state.reads == std::vector<Amounts>{{}});
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
    fixture.first.makeEventsVisible(fixture.context.tick);
    fixture.context.tick++;
    fixture.runTick(fixture.first);

    REQUIRE(state.reads.back() == Amounts{7});
    REQUIRE(TechEngineTests::g_fired.empty());
}
