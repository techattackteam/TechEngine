#include <TechEngine/app/FrameLoop.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

TEST_CASE("sixty fixed-step frames run exactly sixty ticks", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer);

    for (int frame = 0; frame < 60; ++frame) {
        loop.advance(TechEngine::FrameLoop::FIXED_DELTA_TIME);
    }

    REQUIRE(loop.frame().tick == 60);
    REQUIRE(loop.frame().frameIndex == 60);
    REQUIRE(loop.accumulator() == 0.0);
}

TEST_CASE("one frame worth several fixed steps runs them all", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(2.0);

    REQUIRE(loop.frame().tick == 4);
    REQUIRE(loop.frame().frameIndex == 1);
}

TEST_CASE("sub-step frames accumulate until a step is due", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.25);

    REQUIRE(loop.frame().tick == 0);
    REQUIRE(loop.frame().alpha == Catch::Approx(0.5));

    loop.advance(0.25);

    REQUIRE(loop.frame().tick == 1);
    REQUIRE(loop.frame().alpha == Catch::Approx(0.0));
    REQUIRE(loop.frame().frameIndex == 2);
}

TEST_CASE("frameIndex counts frames, not ticks", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer, 0.5, 10.0);

    loop.advance(0.1);
    loop.advance(0.1);
    loop.advance(0.1);

    REQUIRE(loop.frame().frameIndex == 3);
    REQUIRE(loop.frame().tick == 0);
}

TEST_CASE("the loop's state comes only from the deltas fed in", "[app][loop]") {
    const std::array<double, 8> deltas{0.004, 0.021, 0.0166, 0.033, 0.008, 0.05, 0.0009, 0.017};

    TechEngine::FrameLoop first(TechEngine::Role::DedicatedServer);
    TechEngine::FrameLoop second(TechEngine::Role::DedicatedServer);

    for (const double delta : deltas) {
        first.advance(delta);
    }
    for (const double delta : deltas) {
        second.advance(delta);
    }

    REQUIRE(first.frame().tick == second.frame().tick);
    REQUIRE(first.accumulator() == second.accumulator());
}

TEST_CASE("a stalled frame is clamped to the ceiling", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer, 0.5, 2.0);

    loop.advance(60.0);

    REQUIRE(loop.frame().tick == 4);
    REQUIRE(loop.frame().deltaTime == Catch::Approx(2.0));
}

TEST_CASE("repeated stalls never grow the accumulator", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer, 0.5, 2.0);

    for (int frame = 0; frame < 10; ++frame) {
        loop.advance(60.0);
        REQUIRE(loop.accumulator() < 0.5);
    }

    REQUIRE(loop.frame().tick == 40);
}

TEST_CASE("the default clamp bounds catch-up at fifteen ticks", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer);

    loop.advance(2.0);

    // 14 or 15, not exactly 15: 1/60 has no exact binary form, so the last step of
    // 0.25 sits inside the rounding error. The bound is what matters, not the digit.
    REQUIRE(loop.frame().tick >= 14);
    REQUIRE(loop.frame().tick <= 15);
    REQUIRE(loop.accumulator() < TechEngine::FrameLoop::FIXED_DELTA_TIME);
}

TEST_CASE("alpha stays in [0, 1) across ragged frames", "[app][loop]") {
    const std::array<double, 9> deltas{0.004, 0.0166, 0.5, 0.021, 2.0, 0.0009, 0.033, 0.008, 0.05};

    TechEngine::FrameLoop loop(TechEngine::Role::DedicatedServer);

    for (const double delta : deltas) {
        const TechEngine::FrameContext& frame = loop.advance(delta);

        REQUIRE(frame.alpha >= 0.0F);
        REQUIRE(frame.alpha < 1.0F);
    }
}

TEST_CASE("the published context carries the construction values", "[app][loop]") {
    TechEngine::FrameLoop loop(TechEngine::Role::ListenServer, 0.5, 2.0);

    loop.advance(0.25);

    REQUIRE(loop.frame().role == TechEngine::Role::ListenServer);
    REQUIRE(loop.frame().fixedDeltaTime == Catch::Approx(0.5));
}
