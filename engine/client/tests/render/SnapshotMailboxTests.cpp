#include <render/SnapshotMailbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>

TEST_CASE("snapshot mailbox is empty until publication and resets between sessions", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    CHECK_FALSE(mailbox.snapshot());
    TechEngine::RenderSnapshot value;
    value.tick = 42;
    mailbox.publish(value);
    value.tick = 99;
    REQUIRE(mailbox.snapshot());
    CHECK(mailbox.snapshot()->tick == 42);
    auto copy = *mailbox.snapshot();
    copy.tick = 123;
    CHECK(mailbox.snapshot()->tick == 42);
    mailbox.reset();
    CHECK_FALSE(mailbox.snapshot());
}

TEST_CASE("snapshot mailbox returns the newest complete value without draining", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    TechEngine::RenderSnapshot value;
    value.tick = 100;
    mailbox.publish(value);
    value.tick = 103;
    mailbox.publish(value);
    CHECK(mailbox.snapshot()->tick == 103);
    CHECK(mailbox.snapshot()->tick == 103);
}

TEST_CASE("snapshot mailbox never tears tick metadata from its payload", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    std::barrier start{2};
    std::atomic<bool> coherent = true;
    {
        std::jthread producer{[&] {
            start.arrive_and_wait();
            for (std::uint64_t i = 1; i <= 20000; i++) {
                TechEngine::RenderSnapshot value;
                value.tick = i;
                value.timeline = i;
                value.tickTime = TechEngine::Clock::TimePoint{std::chrono::seconds{i}};
                value.clearColor.fill(static_cast<float>(i));
                mailbox.publish(value);
            }
        }};
        std::jthread consumer{[&] {
            start.arrive_and_wait();
            std::uint64_t previous = 0;
            for (int i = 0; i < 20000; i++) {
                if (const auto value = mailbox.snapshot()) {
                    if (value->tick < previous || value->timeline != value->tick || value->clearColor[0] != static_cast<float>(value->tick) || value->tickTime != TechEngine::Clock::TimePoint{std::chrono::seconds{value->tick}}) {
                        coherent = false;
                    }
                    previous = value->tick;
                }
            }
        }};
    }
    CHECK(coherent.load());
    CHECK(mailbox.snapshot()->tick == 20000);
}
