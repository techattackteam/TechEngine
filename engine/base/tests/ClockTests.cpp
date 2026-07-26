#include <TechEngine/base/Clock.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("now is monotonic across calls", "[base][clock]") {
    const TechEngine::Clock clock;

    const TechEngine::Clock::TimePoint first = clock.now();
    const TechEngine::Clock::TimePoint second = clock.now();
    std::this_thread::sleep_for(5ms);
    const TechEngine::Clock::TimePoint third = clock.now();

    REQUIRE(second >= first);
    REQUIRE(third > second);
}

TEST_CASE("totalTime starts near zero and accumulates", "[base][clock]") {
    const TechEngine::Clock clock;

    REQUIRE(clock.totalTime() >= 0.0);

    std::this_thread::sleep_for(5ms);
    const double elapsed = clock.totalTime();

    REQUIRE(elapsed >= 0.004);
    REQUIRE(clock.totalTime() >= elapsed);
}

TEST_CASE("each clock measures from its own construction", "[base][clock]") {
    const TechEngine::Clock first;
    std::this_thread::sleep_for(5ms);
    const TechEngine::Clock second;

    REQUIRE(first.totalTime() > second.totalTime());
}

TEST_CASE("frame starts at zero and advances by exactly one", "[base][clock]") {
    TechEngine::Clock clock;

    REQUIRE(clock.frame() == 0);

    clock.advanceFrame();
    REQUIRE(clock.frame() == 1);

    for (int i = 0; i < 10; ++i) {
        clock.advanceFrame();
    }
    REQUIRE(clock.frame() == 11);
}

TEST_CASE("advancing the frame does not disturb elapsed time", "[base][clock]") {
    TechEngine::Clock clock;

    const double before = clock.totalTime();
    clock.advanceFrame();

    REQUIRE(clock.totalTime() >= before);
}

TEST_CASE("wallClock tracks the system clock", "[base][clock]") {
    const TechEngine::Clock clock;

    const auto reference = std::chrono::system_clock::now();
    const auto stamp = clock.wallClock();
    const auto drift = std::chrono::abs(stamp - reference);

    REQUIRE(drift < 5s);
}
