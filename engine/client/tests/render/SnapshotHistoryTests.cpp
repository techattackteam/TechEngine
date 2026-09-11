#include <render/SnapshotHistory.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>

using namespace std::chrono_literals;
using TechEngine::Clock;

static TechEngine::RenderSnapshot snapshot(std::uint64_t tick, std::chrono::milliseconds time, float color) {
    TechEngine::RenderSnapshot result;
    result.tick = tick;
    result.tickTime = Clock::TimePoint{time};
    result.fixedDeltaTime = 0.1;
    result.timeline = 1;
    result.clearColor.fill(color);
    return result;
}

TEST_CASE("render history interpolates actual timestamp gaps and holds endpoints", "[client][history]") {
    TechEngine::SnapshotHistory history;
    CHECK_FALSE(history.prepareFrame(Clock::TimePoint{}, {}).drawTriangle);
    history.acquire(snapshot(100, 1000ms, 0.0F));
    CHECK(history.alpha(Clock::TimePoint{1150ms}) == 1.0);
    history.acquire(snapshot(103, 1300ms, 1.0F));
    CHECK(history.alpha(Clock::TimePoint{1250ms}) == Catch::Approx(0.5));
    CHECK(history.prepareFrame(Clock::TimePoint{1250ms}, {}).clearColor[0] == Catch::Approx(0.5));
    CHECK(history.alpha(Clock::TimePoint{1000ms}) == 0.0);
    CHECK(history.alpha(Clock::TimePoint{9000ms}) == 1.0);
    history.acquire(snapshot(103, 1300ms, 0.2F));
    history.acquire(snapshot(102, 1200ms, 0.2F));
    CHECK(history.prepareFrame(Clock::TimePoint{1250ms}, {}).clearColor[0] == Catch::Approx(0.5));
}

TEST_CASE("render history resets on a timeline change and ignores stale generations", "[client][history]") {
    TechEngine::SnapshotHistory history;
    history.acquire(snapshot(100, 1000ms, 0.0F));
    history.acquire(snapshot(103, 1300ms, 1.0F));
    auto reset = snapshot(0, 2000ms, 0.25F);
    reset.timeline = 2;
    history.acquire(reset);
    history.acquire(snapshot(999, 9000ms, 1.0F));
    CHECK(history.alpha(Clock::TimePoint{2100ms}) == 1.0);
    CHECK(history.prepareFrame(Clock::TimePoint{2100ms}, {}).clearColor[0] == 0.25F);
}

TEST_CASE("latest cumulative look survives skipped reads without double application", "[client][history][input]") {
    TechEngine::SnapshotHistory history;
    auto value = snapshot(1, 1000ms, 0.0F);
    value.input.focusGeneration = 1;
    value.input.sequence = 10;
    value.input.lookX = 5.0;
    history.acquire(value);
    auto input = value.input;
    input.sequence = 12;
    input.lookX = 9.0;
    CHECK(history.prepareFrame(Clock::TimePoint{1100ms}, input).input.lookX == 9.0);
    CHECK(history.prepareFrame(Clock::TimePoint{1100ms}, input).input.lookX == 9.0);
    value.tick++;
    value.tickTime += 100ms;
    value.input = input;
    history.acquire(value);
    CHECK(history.prepareFrame(Clock::TimePoint{1200ms}, input).input.lookX == 9.0);
    input.focusGeneration++;
    input.sequence++;
    input.lookX = 0.0;
    CHECK(history.prepareFrame(Clock::TimePoint{1200ms}, input).input.lookX == 0.0);
}
