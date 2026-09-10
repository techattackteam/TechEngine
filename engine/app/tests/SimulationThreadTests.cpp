#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/core/EngineContext.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace {
    struct LoopEngine {
        TechEngine::MountTable mounts;
        TechEngine::FileAccess files{mounts};
        TechEngine::JobSystem jobs{4};
        TechEngine::EngineContext context{files, jobs};
    };
}

// The loop only carries the context to its hook; no case here reads through it, so one empty
// mount table serves the whole suite.
static LoopEngine g_loopEngine;

TEST_CASE("loop rates count executed ticks", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.25, 2.0);
    CHECK(loop.ticksPerSecond() == 0.0);
    CHECK_FALSE(loop.ratesUpdated());

    for (int i = 0; i < 8; i++) {
        loop.advance(0.125);
        CHECK(loop.ratesUpdated() == (i == 7));
    }
    CHECK(loop.ticksPerSecond() == Catch::Approx(4.0));

    loop.advance(0.5);
    CHECK_FALSE(loop.ratesUpdated());
    CHECK(loop.ticksPerSecond() == Catch::Approx(4.0));
    loop.advance(0.5);
    CHECK(loop.ratesUpdated());
    CHECK(loop.ticksPerSecond() == Catch::Approx(4.0));
}

TEST_CASE("loop rates include stalled time beyond the simulation clamp", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.125, 0.25);
    loop.advance(2.0);
    REQUIRE(loop.ratesUpdated());
    CHECK(loop.step().tick == 2);
    CHECK(loop.ticksPerSecond() == Catch::Approx(1.0));
}

TEST_CASE("zero and negative deltas do not publish or subtract elapsed rate time", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.25, 2.0);
    loop.advance(0.5);
    loop.advance(0.0);
    loop.advance(-1.0);
    CHECK_FALSE(loop.ratesUpdated());
    CHECK(loop.ticksPerSecond() == 0.0);
    loop.advance(0.5);
    REQUIRE(loop.ratesUpdated());
    CHECK(loop.ticksPerSecond() == Catch::Approx(4.0));
}

TEST_CASE("sixty fixed-step frames run exactly sixty ticks", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    for (int frame = 0; frame < 60; ++frame) {
        loop.advance(TechEngine::SimulationThread::FIXED_DELTA_TIME);
    }

    REQUIRE(loop.step().tick == 60);
    REQUIRE(loop.step().iterationIndex == 60);
    REQUIRE(loop.accumulator() == 0.0);
}

TEST_CASE("one frame worth several fixed steps runs them all", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(2.0);

    REQUIRE(loop.step().tick == 4);
    REQUIRE(loop.step().iterationIndex == 1);
}

TEST_CASE("sub-step frames accumulate until a step is due", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.25);

    REQUIRE(loop.step().tick == 0);
    REQUIRE(loop.step().alpha == Catch::Approx(0.5f));

    loop.advance(0.25);

    REQUIRE(loop.step().tick == 1);
    REQUIRE(loop.step().alpha == Catch::Approx(0.0));
    REQUIRE(loop.step().iterationIndex == 2);
}

TEST_CASE("iterationIndex counts simulation iterations separately from ticks", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.1);
    loop.advance(0.1);
    loop.advance(0.1);

    REQUIRE(loop.step().iterationIndex == 3);
    REQUIRE(loop.step().tick == 0);
}

TEST_CASE("the loop's state comes only from the deltas fed in", "[app][loop]") {
    const std::array<double, 8> deltas{0.004, 0.021, 0.0166, 0.033, 0.008, 0.05, 0.0009, 0.017};

    TechEngine::SimulationThread first(g_loopEngine.context, TechEngine::Role::DedicatedServer);
    TechEngine::SimulationThread second(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    for (const double delta: deltas) {
        first.advance(delta);
    }
    for (const double delta: deltas) {
        second.advance(delta);
    }

    REQUIRE(first.step().tick == second.step().tick);
    REQUIRE(first.accumulator() == second.accumulator());
}

TEST_CASE("a stalled frame is clamped to the ceiling", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 2.0);

    loop.advance(60.0);

    REQUIRE(loop.step().tick == 4);
    REQUIRE(loop.step().deltaTime == Catch::Approx(2.0));
}

TEST_CASE("repeated stalls never grow the accumulator", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 2.0);

    for (int frame = 0; frame < 10; ++frame) {
        loop.advance(60.0);
        REQUIRE(loop.accumulator() < 0.5);
    }

    REQUIRE(loop.step().tick == 40);
}

TEST_CASE("the default clamp bounds catch-up at fifteen ticks", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    loop.advance(2.0);

    // 14 or 15, not exactly 15: 1/60 has no exact binary form, so the last step of
    // 0.25 sits inside the rounding error. The bound is what matters, not the digit.
    REQUIRE(loop.step().tick >= 14);
    REQUIRE(loop.step().tick <= 15);
    REQUIRE(loop.accumulator() < TechEngine::SimulationThread::FIXED_DELTA_TIME);
}

TEST_CASE("alpha stays in [0, 1) across ragged frames", "[app][loop]") {
    const std::array<double, 9> deltas{0.004, 0.0166, 0.5, 0.021, 2.0, 0.0009, 0.033, 0.008, 0.05};

    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer);

    for (const double delta: deltas) {
        const TechEngine::SimulationContext& frame = loop.advance(delta);

        REQUIRE(frame.alpha >= 0.0f);
        REQUIRE(frame.alpha < 1.0f);
    }
}

TEST_CASE("a zero delta advances the frame but not the tick", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.25);
    const std::uint64_t tickBefore = loop.step().tick;
    const double accumulatorBefore = loop.accumulator();

    loop.advance(0.0);

    REQUIRE(loop.step().tick == tickBefore);
    REQUIRE(loop.step().iterationIndex == 2);
    REQUIRE(loop.step().deltaTime == Catch::Approx(0.0));
    REQUIRE(loop.accumulator() == Catch::Approx(accumulatorBefore));
}

TEST_CASE("a negative delta is clamped to zero", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.25);
    const std::uint64_t tickBefore = loop.step().tick;
    const double accumulatorBefore = loop.accumulator();

    loop.advance(-1.0);

    REQUIRE(loop.step().tick == tickBefore);
    REQUIRE(loop.accumulator() >= 0.0);
    REQUIRE(loop.accumulator() == Catch::Approx(accumulatorBefore));
    REQUIRE(loop.step().deltaTime >= 0.0f);
    REQUIRE(loop.step().alpha >= 0.0f);
}

// The hook stamps event batches with the frame and tick it reads here, so a stale iterationIndex
// would retire every batch one frame early. The one-argument overload cannot catch it: by the
// time advance() returns, both orderings have produced the same state.
TEST_CASE("the fixed-step hook sees the frame it is running in", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);
    std::vector<std::uint64_t> frames;
    std::vector<std::uint64_t> ticks;

    const auto record = [&frames, &ticks](const TechEngine::SimulationContext& step) {
        frames.push_back(step.iterationIndex);
        ticks.push_back(step.tick);
    };

    loop.advance(0.5, record);
    loop.advance(0.5, record);

    REQUIRE(frames == std::vector<std::uint64_t>{1, 2});
    REQUIRE(ticks == std::vector<std::uint64_t>{1, 2});
}

TEST_CASE("a catch-up frame runs the hook once per fixed step", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);
    std::vector<std::uint64_t> ticks;

    loop.advance(2.0, [&ticks](const TechEngine::SimulationContext& step) {
        ticks.push_back(step.tick);

        REQUIRE(step.iterationIndex == 1);
        REQUIRE(step.deltaTime == Catch::Approx(2.0));
    });

    REQUIRE(ticks == std::vector<std::uint64_t>{1, 2, 3, 4});
}

TEST_CASE("a frame with no fixed step never runs the hook", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::DedicatedServer, 0.5, 10.0);
    int invocations = 0;

    loop.advance(0.25, [&invocations](const TechEngine::SimulationContext&) {
        invocations++;
    });

    REQUIRE(invocations == 0);
    REQUIRE(loop.step().iterationIndex == 1);
    REQUIRE(loop.step().tick == 0);
}

TEST_CASE("the published context carries the construction values", "[app][loop]") {
    TechEngine::SimulationThread loop(g_loopEngine.context, TechEngine::Role::ListenServer, 0.5, 2.0);

    loop.advance(0.25);

    REQUIRE(loop.step().role == TechEngine::Role::ListenServer);
    REQUIRE(loop.step().fixedDeltaTime == Catch::Approx(0.5));
}
