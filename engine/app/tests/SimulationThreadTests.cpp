#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/InputNotification.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using TechEngine::Clock;
using TechEngine::SimulationContext;
using TechEngine::SimulationSettings;
using TechEngine::SimulationThread;

namespace {
    struct LoopEngine {
        TechEngine::MountTable mounts;
        TechEngine::FileAccess files{mounts};
        TechEngine::JobSystem jobs{4};
        Clock clock;
        TechEngine::EngineContext context{files, jobs, clock};
    };
}

// The loop only carries the context to its hook; no case here reads files through it, so one
// empty mount table and one shared clock serve the whole suite.
static LoopEngine g_loopEngine;

static double toSeconds(Clock::TimePoint::duration duration) {
    return std::chrono::duration<double>(duration).count();
}

static TechEngine::InputEvent keyEvent(TechEngine::Key key, bool pressed) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Key, .key = key, .pressed = pressed};
}

static TechEngine::InputEvent focusEvent(bool focused) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Focus, .pressed = focused};
}

class InputDeliveryProbeSystem final : public TechEngine::ISystem {
public:
    static inline std::vector<std::uint64_t> sequences;
    static inline std::size_t holds = 0;

    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene&, const TechEngine::InputNotification& input) {
            if (input.kind == TechEngine::InputNotificationKind::KeyHold || input.kind == TechEngine::InputNotificationKind::ButtonHold) {
                holds++;
                return;
            }
            sequences.push_back(input.sequence);
        });
    }

    void tick(TechEngine::Scene&, const SimulationContext&) override {
    }

    std::string_view name() const override {
        return "InputDeliveryProbeSystem";
    }
};

class LoopInputBarrier final : public TechEngine::TickBarrierServices {
public:
    void assignNetIds(TechEngine::Scene&, std::span<const TechEngine::Entity>) override {
    }
};

class LoopInputDelivery {
public:
    TechEngine::ComponentRegistry registry;
    TechEngine::EventRegistry events;
    TechEngine::Scene scene;
    TechEngine::Schedule schedule;
    std::optional<TechEngine::TaskGraph> graph;
    std::optional<TechEngine::SerialExecutor> executor;
    LoopInputBarrier barrier;
    std::vector<std::vector<std::uint64_t>> deliveredPerTick;
    std::vector<std::size_t> holdsPerTick;

    LoopInputDelivery() : scene(registry), schedule(registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        InputDeliveryProbeSystem::sequences.clear();
        InputDeliveryProbeSystem::holds = 0;
        schedule.add<InputDeliveryProbeSystem>();
        graph.emplace(schedule, events);
        executor.emplace(*graph);
    }

    void execute(const SimulationContext& simulation) {
        executor->execute(scene, simulation, barrier);
        deliveredPerTick.push_back(std::exchange(InputDeliveryProbeSystem::sequences, {}));
        holdsPerTick.push_back(std::exchange(InputDeliveryProbeSystem::holds, 0));
    }
};

TEST_CASE("sixty fixed-step advances run exactly sixty ticks", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    for (int i = 0; i < 60; i++) {
        loop.advance(SimulationSettings::FIXED_DELTA_TIME);
    }

    REQUIRE(loop.simulationContext().tick == 60);
    REQUIRE(loop.accumulator() == 0.0);
}

TEST_CASE("one advance worth several fixed steps runs each tick in order", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0});
    std::vector<std::uint64_t> ticks;

    loop.advance(2.0, [&ticks](const SimulationContext& simulation) {
        ticks.push_back(simulation.tick);
    });

    REQUIRE(ticks == std::vector<std::uint64_t>{1, 2, 3, 4});
}

TEST_CASE("sub-step advances accumulate until a step is due", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0});
    int invocations = 0;
    const auto count = [&invocations](const SimulationContext&) {
        invocations++;
    };

    loop.advance(0.25, count);
    REQUIRE(invocations == 0);
    REQUIRE(loop.simulationContext().tick == 0);
    REQUIRE(loop.accumulator() == Catch::Approx(0.25));

    loop.advance(0.25, count);
    REQUIRE(invocations == 1);
    REQUIRE(loop.simulationContext().tick == 1);
    REQUIRE(loop.accumulator() == Catch::Approx(0.0));
}

TEST_CASE("the simulation's state comes only from the elapsed time fed in", "[app][loop]") {
    const std::array<double, 8> deltas{0.004, 0.021, 0.0166, 0.033, 0.008, 0.05, 0.0009, 0.017};

    SimulationThread first(g_loopEngine.context, TechEngine::Role::DedicatedServer);
    SimulationThread second(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    for (const double delta: deltas) {
        first.advance(delta);
    }
    for (const double delta: deltas) {
        second.advance(delta);
    }

    REQUIRE(first.simulationContext().tick == second.simulationContext().tick);
    REQUIRE(first.accumulator() == second.accumulator());
}

TEST_CASE("a stall is clamped without skipping tick numbers", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 2.0});
    const std::uint64_t timeline = loop.simulationContext().timeline;
    std::vector<std::uint64_t> ticks;

    loop.advance(60.0, [&ticks](const SimulationContext& simulation) {
        ticks.push_back(simulation.tick);
    });

    REQUIRE(ticks == std::vector<std::uint64_t>{1, 2, 3, 4});
    CHECK(loop.simulationContext().timeline == timeline + 1);
}

TEST_CASE("repeated stalls never grow the accumulator", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 2.0});

    for (int i = 0; i < 10; i++) {
        loop.advance(60.0);
        REQUIRE(loop.accumulator() < 0.5);
    }

    REQUIRE(loop.simulationContext().tick == 40);
}

TEST_CASE("the default clamp bounds catch-up at fifteen ticks", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    loop.advance(2.0);

    // 14 or 15, not exactly 15: 1/60 has no exact binary form, so the last step of
    // 0.25 sits inside the rounding error. The bound is what matters, not the digit.
    REQUIRE(loop.simulationContext().tick >= 14);
    REQUIRE(loop.simulationContext().tick <= 15);
    REQUIRE(loop.accumulator() < SimulationSettings::FIXED_DELTA_TIME);
}

TEST_CASE("negative and non-finite elapsed time count as zero", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0});
    loop.advance(0.25);
    const std::uint64_t timeline = loop.simulationContext().timeline;

    loop.advance(-1.0);
    loop.advance(std::numeric_limits<double>::quiet_NaN());
    loop.advance(std::numeric_limits<double>::infinity());

    REQUIRE(loop.simulationContext().tick == 0);
    REQUIRE(loop.simulationContext().timeline == timeline);
    REQUIRE(loop.accumulator() == Catch::Approx(0.25));
}

TEST_CASE("tick boundary times stay one fixed step apart through catch-up", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0});
    const Clock::TimePoint origin{1000s};
    loop.restartTimeline(origin);
    std::vector<Clock::TimePoint> boundaries;

    loop.advance(1.75, [&boundaries](const SimulationContext& simulation) {
        boundaries.push_back(simulation.tickTime);
    });

    REQUIRE(boundaries.size() == 3);
    CHECK(toSeconds(boundaries[0] - origin) == Catch::Approx(0.5));
    CHECK(toSeconds(boundaries[1] - boundaries[0]) == Catch::Approx(0.5));
    CHECK(toSeconds(boundaries[2] - boundaries[1]) == Catch::Approx(0.5));
    CHECK(loop.simulationContext().tickTime == boundaries.back());
}

TEST_CASE("a clamped stall shifts later boundaries and starts a new timeline", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 1.0});
    const Clock::TimePoint origin{1000s};
    loop.restartTimeline(origin);
    loop.advance(1.0);
    const std::uint64_t timeline = loop.simulationContext().timeline;
    REQUIRE(loop.simulationContext().tick == 2);

    loop.advance(3.0);

    CHECK(loop.simulationContext().tick == 4);
    CHECK(loop.simulationContext().timeline == timeline + 1);
    CHECK(toSeconds(loop.simulationContext().tickTime - origin) == Catch::Approx(4.0));
}

TEST_CASE("tick rates count completed ticks over unclamped elapsed time", "[app][loop]") {
    SECTION("steady advances") {
        SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.25, .maxElapsedTime = 2.0});
        for (int i = 0; i < 8; i++) {
            loop.advance(0.125);
        }
        CHECK(loop.timing().tick == 4);
        CHECK(loop.timing().ticksPerSecond == Catch::Approx(4.0));
    }
    SECTION("a stall beyond the clamp") {
        SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.125, .maxElapsedTime = 0.25});
        loop.advance(2.0);
        CHECK(loop.timing().tick == 2);
        CHECK(loop.timing().ticksPerSecond == Catch::Approx(1.0));
    }
}

TEST_CASE("simulations share one clock but only the primary advances its diagnostic counter", "[app][loop]") {
    Clock diagnostics;
    SimulationThread primary(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .diagnosticClock = &diagnostics});
    SimulationThread secondary(g_loopEngine.context, TechEngine::Role::DedicatedServer, {.fixedDeltaTime = 0.25, .maxElapsedTime = 10.0});

    primary.advance(1.5);
    secondary.advance(1.5);

    CHECK(primary.simulationContext().tick == 3);
    CHECK(secondary.simulationContext().tick == 6);
    CHECK(diagnostics.tick() == 3);
    CHECK(&primary.simulationContext().engine.clock == &secondary.simulationContext().engine.clock);
}

TEST_CASE("input is consumed immediately before each fixed tick", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    std::vector<std::size_t> eventCounts;
    std::vector<bool> held;
    const auto record = [&eventCounts, &held](const SimulationContext& simulation) {
        eventCounts.push_back(simulation.input.events.size());
        held.push_back(simulation.input.held.isHeld(TechEngine::Key::W));
    };

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    input.publish(keyEvent(TechEngine::Key::W, false));
    loop.advance(1.0, record);

    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(0.25, record);
    loop.advance(0.25, record);

    loop.advance(1.5, record);

    input.publish(focusEvent(false));
    loop.advance(0.5, record);

    CHECK(eventCounts == std::vector<std::size_t>{3, 0, 1, 0, 0, 0, 1});
    CHECK(held == std::vector<bool>{false, false, true, true, true, true, false});
}

TEST_CASE("captured input reaches no handler until a Tick consumes it", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    LoopInputDelivery delivery;
    const auto step = [&delivery](const SimulationContext& simulation) {
        delivery.execute(simulation);
    };

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(0.25, step);

    REQUIRE(delivery.deliveredPerTick.empty());

    loop.advance(0.25, step);

    REQUIRE(delivery.deliveredPerTick == std::vector<std::vector<std::uint64_t>>{{1, 2}});
    REQUIRE(delivery.holdsPerTick == std::vector<std::size_t>{1});
}

TEST_CASE("each catch-up Tick delivers only the batch it consumed", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    LoopInputDelivery delivery;

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(1.5, [&delivery, &input](const SimulationContext& simulation) {
        delivery.execute(simulation);
        if (simulation.tick == 1) {
            input.publish(keyEvent(TechEngine::Key::W, false));
        }
    });

    REQUIRE(loop.simulationContext().tick == 3);
    REQUIRE(delivery.deliveredPerTick == std::vector<std::vector<std::uint64_t>>{{1, 2}, {3}, {}});
    REQUIRE(delivery.holdsPerTick == std::vector<std::size_t>{1, 0, 0});
}

TEST_CASE("every catch-up Tick reports a held key once, with or without captured events", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    LoopInputDelivery delivery;
    const auto step = [&delivery](const SimulationContext& simulation) {
        delivery.execute(simulation);
    };

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(1.5, step);

    REQUIRE(delivery.deliveredPerTick == std::vector<std::vector<std::uint64_t>>{{1, 2}, {}, {}});
    REQUIRE(delivery.holdsPerTick == std::vector<std::size_t>{1, 1, 1});
}

TEST_CASE("GLFW's post-loss synthetic releases and a duplicate focus reach no handler and leave no key held", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    LoopInputDelivery delivery;
    const auto step = [&delivery](const SimulationContext& simulation) {
        delivery.execute(simulation);
    };

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(0.5, step);

    input.publish(focusEvent(false));
    input.publish(keyEvent(TechEngine::Key::W, false));
    input.publish(focusEvent(false));
    loop.advance(0.5, step);

    input.publish(focusEvent(true));
    loop.advance(0.5, step);

    REQUIRE(delivery.deliveredPerTick == std::vector<std::vector<std::uint64_t>>{{1, 2}, {3}, {6}});
    REQUIRE(delivery.holdsPerTick == std::vector<std::size_t>{1, 0, 0});
    REQUIRE_FALSE(loop.simulationContext().input.held.isHeld(TechEngine::Key::W));
}

TEST_CASE("a duplicate focus between Ticks keeps a held key reporting holds", "[app][loop][input]") {
    TechEngine::InputBuffer input{g_loopEngine.clock};
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::Client, {.fixedDeltaTime = 0.5, .maxElapsedTime = 10.0, .input = &input});
    LoopInputDelivery delivery;
    const auto step = [&delivery](const SimulationContext& simulation) {
        delivery.execute(simulation);
    };

    input.publish(focusEvent(true));
    input.publish(keyEvent(TechEngine::Key::W, true));
    loop.advance(0.5, step);

    input.publish(focusEvent(true));
    loop.advance(0.5, step);

    REQUIRE(delivery.deliveredPerTick == std::vector<std::vector<std::uint64_t>>{{1, 2}, {}});
    REQUIRE(delivery.holdsPerTick == std::vector<std::size_t>{1, 1});
}

TEST_CASE("the context carries the construction values", "[app][loop]") {
    SimulationThread loop(g_loopEngine.context, TechEngine::Role::ListenServer, {.fixedDeltaTime = 0.5, .maxElapsedTime = 2.0});

    loop.advance(0.25);

    REQUIRE(loop.simulationContext().role == TechEngine::Role::ListenServer);
    REQUIRE(loop.simulationContext().fixedDeltaTime == Catch::Approx(0.5));
}
