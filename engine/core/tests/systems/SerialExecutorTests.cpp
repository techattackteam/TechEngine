#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

struct ExecutorValue {
    int value = 0;
};

enum class ExecutorCommandMode { None, Spawn, Replace, Despawn };
enum class ExecutorAccessMode { None, Read, Write, UndeclaredRead };

struct ExecutorTestState {
    std::vector<std::pair<int, int>> runs;
    std::vector<int> constructed;
    std::vector<int> initialized;
    TechEngine::Entity target;
    TechEngine::PendingEntity pending;
    ExecutorCommandMode commandMode = ExecutorCommandMode::None;
    ExecutorAccessMode accessMode = ExecutorAccessMode::None;
    bool visibleDuringSystem = false;
    bool throwAfterCommand = false;
};

static ExecutorTestState* g_executorState = nullptr;

class ExecutorStateGuard {
public:
    explicit ExecutorStateGuard(ExecutorTestState& state) {
        REQUIRE(g_executorState == nullptr);
        g_executorState = &state;
    }

    ~ExecutorStateGuard() {
        g_executorState = nullptr;
    }

    ExecutorStateGuard(const ExecutorStateGuard&) = delete;

    ExecutorStateGuard& operator=(const ExecutorStateGuard&) = delete;
};

template<int Index>
class OrderedExecutorSystem final : public TechEngine::ISystem {
private:
    int m_runCount = 0;

public:
    OrderedExecutorSystem() {
        g_executorState->constructed.push_back(Index);
    }

    void init(TechEngine::ScheduleRegistration&) override {
        g_executorState->initialized.push_back(Index);
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        m_runCount++;
        g_executorState->runs.emplace_back(Index, m_runCount);
    }

    std::string_view name() const override {
        return "OrderedExecutorSystem";
    }
};

using FirstExecutorSystem = OrderedExecutorSystem<1>;
using SecondExecutorSystem = OrderedExecutorSystem<2>;
using TerminalExecutorSystem = OrderedExecutorSystem<3>;

class CommandExecutorSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        switch (g_executorState->commandMode) {
            case ExecutorCommandMode::None:
                break;
            case ExecutorCommandMode::Spawn: {
                const TechEngine::PendingEntity entity = scene.getCommands().spawn();
                scene.addComponent<ExecutorValue>(entity, 41);
                break;
            }
            case ExecutorCommandMode::Replace:
                scene.removeComponent<ExecutorValue>(g_executorState->target);
                scene.addComponent<ExecutorValue>(g_executorState->target, 99);
                break;
            case ExecutorCommandMode::Despawn:
                scene.getCommands().despawn(g_executorState->target);
                g_executorState->visibleDuringSystem = scene.contains(g_executorState->target);
                break;
        }
    }

    std::string_view name() const override {
        return "CommandExecutorSystem";
    }
};

class AccessExecutorSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        switch (g_executorState->accessMode) {
            case ExecutorAccessMode::None:
                break;
            case ExecutorAccessMode::Read: {
                const TechEngine::Scene& readScene = scene;
                (void)readScene.getComponent<TechEngine::Transform>(g_executorState->target);
                break;
            }
            case ExecutorAccessMode::Write:
                (void)scene.getComponent<TechEngine::Transform>(g_executorState->target);
                break;
            case ExecutorAccessMode::UndeclaredRead:
                (void)scene.hasComponent<ExecutorValue>(g_executorState->target);
                break;
        }
    }

    std::string_view name() const override {
        return "AccessExecutorSystem";
    }
};

class ThrowingExecutorSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        if (!g_executorState->throwAfterCommand) {
            return;
        }
        scene.getCommands().despawn(g_executorState->target);
        throw std::runtime_error("system failure");
    }

    std::string_view name() const override {
        return "ThrowingExecutorSystem";
    }
};

class ImmediateMutationSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        (void)scene.createEntity();
    }

    std::string_view name() const override {
        return "ImmediateMutationSystem";
    }
};

class PendingProducerSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        g_executorState->pending = scene.getCommands().spawn();
    }

    std::string_view name() const override {
        return "PendingProducerSystem";
    }
};

class ForeignPendingConsumerSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        scene.addComponent<ExecutorValue>(g_executorState->pending, 7);
    }

    std::string_view name() const override {
        return "ForeignPendingConsumerSystem";
    }
};

template<int Value>
class BufferedSpawnSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        const TechEngine::PendingEntity entity = scene.getCommands().spawn();
        scene.addComponent<ExecutorValue>(entity, Value);
    }

    std::string_view name() const override {
        return "BufferedSpawnSystem";
    }
};

class BarrierProbe final : public TechEngine::TickBarrierServices {
public:
    std::vector<TechEngine::Entity> spawned;
    std::vector<std::string_view> calls;
    std::uint64_t tick = 0;

    void assignNetIds(TechEngine::Scene&, const std::span<const TechEngine::Entity> entities) override {
        calls.push_back("assignNetIds");
        spawned.assign(entities.begin(), entities.end());
    }

    void flushEvents(const std::uint64_t currentTick) override {
        calls.push_back("flushEvents");
        tick = currentTick;
    }
};

class ExecutorFixture {
public:
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files;
    TechEngine::JobSystem jobs;
    TechEngine::Clock clock;
    TechEngine::EngineContext engine;
    TechEngine::InputFrame input;
    TechEngine::SimulationContext context;

    ExecutorFixture() : scene(registry), files(mounts), jobs(1), engine{files, jobs, clock}, context{.fixedDeltaTime = 1.0 / 60.0, .tick = 1, .input = input, .engine = engine} {
        TechEngineTests::registerBuiltInSceneComponents(registry);
        registry.registerComponent<ExecutorValue>("Test.ExecutorValue");
    }
};

TEST_CASE("scene component mutation is immediate outside system execution", "[core][systems][executor]") {
    ExecutorFixture fixture;
    const TechEngine::Entity entity = fixture.scene.createEntity();

    fixture.scene.addComponent<ExecutorValue>(entity, 17);

    REQUIRE(fixture.scene.hasComponent<ExecutorValue>(entity));
    REQUIRE(fixture.scene.getComponent<ExecutorValue>(entity).value == 17);

    fixture.scene.removeComponent<ExecutorValue>(entity);

    REQUIRE_FALSE(fixture.scene.hasComponent<ExecutorValue>(entity));
}

TEST_CASE("the serial executor walks levels and retains each system instance", "[core][systems][executor]") {
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstExecutorSystem>().before<SecondExecutorSystem>();
    schedule.add<SecondExecutorSystem>();
    schedule.add<TerminalExecutorSystem>().setSlot(TechEngine::Slot::Terminal);
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    REQUIRE(state.constructed == std::vector<int>{1, 2, 3});
    REQUIRE(state.initialized == std::vector<int>{1, 2, 3});
    executor.execute(fixture.scene, fixture.context, barrier);
    fixture.context.tick++;
    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(state.runs == std::vector<std::pair<int, int>>{{1, 1}, {2, 1}, {3, 1}, {1, 2}, {2, 2}, {3, 2}});
}

TEST_CASE("an empty graph still reaches the tick barrier", "[core][systems][executor]") {
    ExecutorFixture fixture;
    TechEngine::Schedule schedule(fixture.registry);
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;
    fixture.context.tick = 19;

    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(barrier.calls == std::vector<std::string_view>{"assignNetIds", "flushEvents"});
    REQUIRE(barrier.spawned.empty());
    REQUIRE(barrier.tick == 19);
}

TEST_CASE("spawn and component commands apply in issue order before barrier services", "[core][systems][executor]") {
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    state.commandMode = ExecutorCommandMode::Spawn;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<CommandExecutorSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(barrier.calls == std::vector<std::string_view>{"assignNetIds", "flushEvents"});
    REQUIRE(barrier.spawned.size() == 1);
    const TechEngine::Entity entity = barrier.spawned.front();
    REQUIRE(fixture.scene.contains(entity));
    REQUIRE(fixture.scene.getComponent<ExecutorValue>(entity).value == 41);

    state.target = entity;
    state.commandMode = ExecutorCommandMode::Replace;
    fixture.context.tick++;
    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(fixture.scene.hasComponent<ExecutorValue>(entity));
    REQUIRE(fixture.scene.getComponent<ExecutorValue>(entity).value == 99);
}

TEST_CASE("per-system command buffers merge in graph order", "[core][systems][executor]") {
    ExecutorFixture fixture;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<BufferedSpawnSystem<10>>();
    schedule.add<BufferedSpawnSystem<20>>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(barrier.spawned.size() == 2);
    REQUIRE(fixture.scene.getComponent<ExecutorValue>(barrier.spawned[0]).value == 10);
    REQUIRE(fixture.scene.getComponent<ExecutorValue>(barrier.spawned[1]).value == 20);
}

TEST_CASE("despawn remains invisible until the barrier and validates the hierarchy on commit", "[core][systems][executor]") {
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    const TechEngine::Entity parent = fixture.scene.createEntity();
    const TechEngine::Entity child = fixture.scene.createEntity();
    REQUIRE(fixture.scene.setParent(child, parent));
    state.target = parent;
    state.commandMode = ExecutorCommandMode::Despawn;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<CommandExecutorSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(state.visibleDuringSystem);
    REQUIRE_FALSE(fixture.scene.contains(parent));
    REQUIRE_FALSE(fixture.scene.contains(child));
}

TEST_CASE("undeclared component access fires the debug assertion without corrupting the execution scope", "[core][systems][executor]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    state.accessMode = ExecutorAccessMode::UndeclaredRead;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<AccessExecutorSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    executor.execute(fixture.scene, fixture.context, barrier);

#if TE_ASSERT_DEV
    REQUIRE(TechEngineTests::g_fired == std::vector<TechEngine::AssertKind>{TechEngine::AssertKind::Assert});
#else
    REQUIRE(TechEngineTests::g_fired.empty());
#endif
    state.accessMode = ExecutorAccessMode::None;
    executor.execute(fixture.scene, fixture.context, barrier);
}

TEST_CASE("write stamps advance for declared writes but not declared reads", "[core][systems][executor]") {
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    TechEngine::Schedule writeSchedule(fixture.registry);
    writeSchedule.add<AccessExecutorSystem>(TechEngine::DeclareAccess<TechEngine::Write<TechEngine::Transform>, TechEngine::Read<>>{});
    const TechEngine::TaskGraph writeGraph(writeSchedule);
    TechEngine::SerialExecutor writer(writeGraph);
    BarrierProbe barrier;
    state.accessMode = ExecutorAccessMode::None;
    fixture.context.tick = 7;

    writer.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(fixture.scene.getChangeTick<TechEngine::Transform>(state.target) == 7);
    REQUIRE(fixture.scene.getChangeTick<ExecutorValue>(state.target) == 0);
    REQUIRE(fixture.scene.getChangeTick<TechEngine::Transform>({}) == 0);

    TechEngine::Schedule readSchedule(fixture.registry);
    readSchedule.add<AccessExecutorSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<TechEngine::Transform>>{});
    const TechEngine::TaskGraph readGraph(readSchedule);
    TechEngine::SerialExecutor reader(readGraph);
    state.accessMode = ExecutorAccessMode::Read;
    fixture.context.tick = 8;

    reader.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(fixture.scene.getChangeTick<TechEngine::Transform>(state.target) == 7);
}

TEST_CASE("a failing system discards every pending command and skips the barrier", "[core][systems][executor]") {
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    state.throwAfterCommand = true;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<ThrowingExecutorSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    REQUIRE_THROWS_AS(executor.execute(fixture.scene, fixture.context, barrier), std::runtime_error);
    REQUIRE(fixture.scene.contains(state.target));
    REQUIRE(barrier.calls.empty());

    state.throwAfterCommand = false;
    executor.execute(fixture.scene, fixture.context, barrier);

    REQUIRE(fixture.scene.contains(state.target));
    REQUIRE(barrier.calls == std::vector<std::string_view>{"assignNetIds", "flushEvents"});
}

TEST_CASE("immediate structural mutation is rejected during system execution", "[core][systems][executor]") {
    const TechEngineTests::FatalAssertGuard assertGuard;
    ExecutorFixture fixture;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<ImmediateMutationSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    REQUIRE_THROWS_AS(executor.execute(fixture.scene, fixture.context, barrier), TechEngineTests::AssertFired);
    REQUIRE(barrier.calls.empty());
}

TEST_CASE("a pending entity cannot cross per-system command buffers", "[core][systems][executor]") {
    const TechEngineTests::FatalAssertGuard assertGuard;
    ExecutorFixture fixture;
    ExecutorTestState state;
    const ExecutorStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<PendingProducerSystem>().before<ForeignPendingConsumerSystem>();
    schedule.add<ForeignPendingConsumerSystem>();
    const TechEngine::TaskGraph graph(schedule);
    TechEngine::SerialExecutor executor(graph);
    BarrierProbe barrier;

    REQUIRE_THROWS_AS(executor.execute(fixture.scene, fixture.context, barrier), TechEngineTests::AssertFired);
    REQUIRE(barrier.calls.empty());
}
