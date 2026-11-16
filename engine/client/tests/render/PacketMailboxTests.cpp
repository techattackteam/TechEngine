#include <render/FramePacketPool.hpp>
#include <render/FrameView.hpp>
#include <render/PacketHistory.hpp>
#include <render/PacketMailbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>

using TechEngine::Clock;
using TechEngine::FramePacket;
using TechEngine::FramePacketPool;
using TechEngine::FrameView;
using TechEngine::PacketHistory;
using TechEngine::PacketMailbox;

static std::unique_ptr<FramePacket> packetAt(FramePacketPool& pool, const std::uint64_t tick) {
    std::unique_ptr<FramePacket> packet = pool.acquire();
    REQUIRE(packet != nullptr);
    packet->tick = tick;
    packet->tickTime = Clock::TimePoint{std::chrono::milliseconds{100 * tick}};
    packet->fixedDeltaTime = 0.1;
    packet->timeline = 1;
    return packet;
}

static std::uint64_t currentTick(const PacketHistory& history) {
    FrameView view;
    history.prepareFrame(Clock::TimePoint{}, {}, view);
    REQUIRE(view.packet != nullptr);
    return view.packet->tick;
}

TEST_CASE("packet mailbox is empty until publication, drains on delivery and resets between sessions", "[client][packet]") {
    FramePacketPool pool;
    PacketMailbox mailbox{pool};
    PacketHistory history{pool};
    CHECK_FALSE(mailbox.deliverTo(history));

    mailbox.publish(packetAt(pool, 42));
    REQUIRE(mailbox.deliverTo(history));
    CHECK(currentTick(history) == 42);
    CHECK_FALSE(mailbox.deliverTo(history));
    CHECK(currentTick(history) == 42);

    mailbox.publish(packetAt(pool, 43));
    CHECK(pool.getAvailableCount() == 2);
    mailbox.reset();
    CHECK(pool.getAvailableCount() == 3);
    CHECK_FALSE(mailbox.deliverTo(history));
}

TEST_CASE("packet mailbox leaves the history untouched while empty", "[client][packet]") {
    FramePacketPool pool;
    PacketMailbox mailbox{pool};
    PacketHistory history{pool};
    FrameView view;
    history.prepareFrame(Clock::TimePoint{}, {}, view);
    CHECK(view.packet == nullptr);
    CHECK_FALSE(mailbox.deliverTo(history));
    history.prepareFrame(Clock::TimePoint{}, {}, view);
    CHECK(view.packet == nullptr);
}

TEST_CASE("a publish replaces an unread packet and returns it to the pool", "[client][packet]") {
    FramePacketPool pool;
    PacketMailbox mailbox{pool};
    PacketHistory history{pool};
    mailbox.publish(packetAt(pool, 100));
    mailbox.publish(packetAt(pool, 103));
    CHECK(pool.getAvailableCount() == 3);
    REQUIRE(mailbox.deliverTo(history));
    CHECK(currentTick(history) == 103);
}

TEST_CASE("packet mailbox never tears tick metadata from its payload", "[client][packet]") {
    FramePacketPool pool;
    PacketMailbox mailbox{pool};
    PacketHistory history{pool};
    std::barrier start{2};
    std::atomic<bool> coherent = true;
    std::atomic<bool> producing = true;
    {
        std::jthread producer{[&] {
            start.arrive_and_wait();
            for (std::uint64_t i = 1; i <= 20000; i++) {
                std::unique_ptr<FramePacket> packet = pool.acquire();
                if (packet == nullptr) {
                    coherent = false;
                    break;
                }
                packet->tick = i;
                packet->timeline = i;
                packet->tickTime = Clock::TimePoint{std::chrono::seconds{i}};
                packet->clearColor.fill(static_cast<float>(i));
                mailbox.publish(std::move(packet));
            }
            producing.store(false);
        }};
        std::jthread consumer{[&] {
            start.arrive_and_wait();
            std::uint64_t previous = 0;
            FrameView view;
            while (producing.load()) {
                mailbox.deliverTo(history);
                history.prepareFrame(Clock::TimePoint{}, {}, view);
                if (view.packet != nullptr) {
                    const FramePacket& value = *view.packet;
                    if (value.tick < previous || value.timeline != value.tick || value.clearColor[0] != static_cast<float>(value.tick) || value.tickTime != Clock::TimePoint{std::chrono::seconds{value.tick}}) {
                        coherent = false;
                    }
                    previous = value.tick;
                }
            }
        }};
    }
    CHECK(coherent.load());
    mailbox.deliverTo(history);
    CHECK(currentTick(history) == 20000);
}
