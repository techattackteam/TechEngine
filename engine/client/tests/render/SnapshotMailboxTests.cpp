#include <render/SnapshotMailbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>

TEST_CASE("snapshot mailbox is empty until publication and resets between sessions", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    TechEngine::RenderSnapshot empty;
    CHECK_FALSE(mailbox.snapshot(empty));
    TechEngine::RenderSnapshot value;
    value.tick = 42;
    mailbox.publish(value);
    value.tick = 99;
    TechEngine::RenderSnapshot copy;
    REQUIRE(mailbox.snapshot(copy));
    CHECK(copy.tick == 42);
    copy.tick = 123;
    TechEngine::RenderSnapshot again;
    REQUIRE(mailbox.snapshot(again));
    CHECK(again.tick == 42);
    mailbox.reset();
    TechEngine::RenderSnapshot afterReset;
    CHECK_FALSE(mailbox.snapshot(afterReset));
}

TEST_CASE("snapshot mailbox leaves the output untouched while empty", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    TechEngine::RenderSnapshot out;
    out.tick = 77;
    CHECK_FALSE(mailbox.snapshot(out));
    CHECK(out.tick == 77);
}

TEST_CASE("snapshot mailbox returns the newest complete value without draining", "[client][snapshot]") {
    TechEngine::SnapshotMailbox mailbox;
    TechEngine::RenderSnapshot value;
    value.tick = 100;
    mailbox.publish(value);
    value.tick = 103;
    mailbox.publish(value);
    TechEngine::RenderSnapshot first;
    REQUIRE(mailbox.snapshot(first));
    CHECK(first.tick == 103);
    TechEngine::RenderSnapshot second;
    REQUIRE(mailbox.snapshot(second));
    CHECK(second.tick == 103);
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
                TechEngine::RenderSnapshot value;
                const bool received = mailbox.snapshot(value);
                if (received) {
                    if (value.tick < previous || value.timeline != value.tick || value.clearColor[0] != static_cast<float>(value.tick) || value.tickTime != TechEngine::Clock::TimePoint{std::chrono::seconds{value.tick}}) {
                        coherent = false;
                    }
                    previous = value.tick;
                }
            }
        }};
    }
    CHECK(coherent.load());
    TechEngine::RenderSnapshot last;
    REQUIRE(mailbox.snapshot(last));
    CHECK(last.tick == 20000);
}
