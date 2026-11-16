#include <TechEngine/testing/AssertCapture.hpp>

#include <render/FramePacketPool.hpp>
#include <render/FrameView.hpp>
#include <render/PacketHistory.hpp>
#include <render/PacketMailbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using TechEngine::Clock;
using TechEngine::FramePacket;
using TechEngine::FramePacketPool;

static void stamp(FramePacket& packet, const std::uint64_t tick) {
    packet.tick = tick;
    packet.tickTime = Clock::TimePoint{std::chrono::milliseconds{100 * tick}};
    packet.fixedDeltaTime = 0.1;
    packet.timeline = 1;
}

TEST_CASE("the pool holds four packets", "[client][packet]") {
    FramePacketPool pool;
    CHECK(FramePacketPool::PACKET_COUNT == 4);
    CHECK(pool.getAvailableCount() == 4);

    std::vector<std::unique_ptr<FramePacket>> taken;
    std::set<const FramePacket*> distinct;
    for (std::size_t i = 0; i < FramePacketPool::PACKET_COUNT; i++) {
        taken.push_back(pool.acquire());
        REQUIRE(taken.back() != nullptr);
        distinct.insert(taken.back().get());
        CHECK(pool.getAvailableCount() == FramePacketPool::PACKET_COUNT - i - 1);
    }
    CHECK(distinct.size() == 4);

    for (auto& packet: taken) {
        pool.release(std::move(packet));
    }
    CHECK(pool.getAvailableCount() == 4);
}

TEST_CASE("an empty pool fires one check and hands out nothing", "[client][packet]") {
    const TechEngineTests::AssertHandlerGuard asserts;
    FramePacketPool pool;
    std::vector<std::unique_ptr<FramePacket>> taken;
    for (std::size_t i = 0; i < FramePacketPool::PACKET_COUNT; i++) {
        taken.push_back(pool.acquire());
    }
    REQUIRE(TechEngineTests::g_fired.empty());

    CHECK(pool.acquire() == nullptr);
    REQUIRE(TechEngineTests::g_fired.size() == 1);
    CHECK(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Check);
    CHECK(pool.getAvailableCount() == 0);

    pool.release(std::move(taken.back()));
    CHECK(pool.acquire() != nullptr);
}

TEST_CASE("releasing no packet changes nothing", "[client][packet]") {
    FramePacketPool pool;
    pool.release(nullptr);
    CHECK(pool.getAvailableCount() == 4);
}

TEST_CASE("a released packet keeps the capacity its vectors grew to", "[client][packet]") {
    FramePacketPool pool;
    std::unique_ptr<FramePacket> grown = pool.acquire();
    REQUIRE(grown != nullptr);
    grown->instances.resize(256);
    grown->materials.resize(512);
    const FramePacket* address = grown.get();
    pool.release(std::move(grown));

    std::vector<std::unique_ptr<FramePacket>> taken;
    for (std::size_t i = 0; i < FramePacketPool::PACKET_COUNT; i++) {
        taken.push_back(pool.acquire());
    }
    bool found = false;
    for (const auto& packet: taken) {
        if (packet.get() == address) {
            found = true;
            CHECK(packet->instances.capacity() >= 256);
            CHECK(packet->materials.capacity() >= 512);
        }
    }
    CHECK(found);
}

TEST_CASE("simulation never waits with four packets, whatever render does between publishes", "[client][packet]") {
    const TechEngineTests::AssertHandlerGuard asserts;
    // How many times render delivers after each publish: never, once, twice, then mixed.
    const std::vector<std::vector<int>> patterns{{0}, {1}, {2}, {0, 0, 1, 2, 0, 1, 1, 0, 2}};
    for (const auto& pattern: patterns) {
        FramePacketPool pool;
        std::size_t emptyAcquires = 0;
        TechEngine::PacketHistory history{pool};
        TechEngine::PacketMailbox mailbox{pool};
        TechEngine::FrameView frame;
        for (std::uint64_t tick = 1; tick <= 200; tick++) {
            std::unique_ptr<FramePacket> packet = pool.acquire();
            if (packet == nullptr) {
                emptyAcquires++;
                continue;
            }
            stamp(*packet, tick);
            mailbox.publish(std::move(packet));
            const int deliveries = pattern[tick % pattern.size()];
            for (int i = 0; i < deliveries; i++) {
                mailbox.deliverTo(history);
                history.prepareFrame(Clock::TimePoint{std::chrono::milliseconds{100 * tick}}, {}, frame);
            }
        }
        CHECK(emptyAcquires == 0);
        CHECK(TechEngineTests::g_fired.empty());
    }
}

TEST_CASE("simulation never waits with four packets while render delivers on its own thread", "[client][packet]") {
    const TechEngineTests::AssertHandlerGuard asserts;
    FramePacketPool pool;
    std::atomic<std::size_t> emptyAcquires = 0;
    std::atomic<bool> producing = true;
    std::barrier start{2};
    {
        TechEngine::PacketMailbox mailbox{pool};
        TechEngine::PacketHistory history{pool};
        std::jthread render{[&] {
            TechEngine::FrameView frame;
            start.arrive_and_wait();
            while (producing.load()) {
                mailbox.deliverTo(history);
                history.prepareFrame(Clock::TimePoint{}, {}, frame);
            }
        }};
        std::jthread simulation{[&] {
            start.arrive_and_wait();
            for (std::uint64_t tick = 1; tick <= 20000; tick++) {
                std::unique_ptr<FramePacket> packet = pool.acquire();
                if (packet == nullptr) {
                    emptyAcquires++;
                    continue;
                }
                stamp(*packet, tick);
                mailbox.publish(std::move(packet));
            }
            producing.store(false);
        }};
    }
    CHECK(emptyAcquires.load() == 0);
    CHECK(pool.getAvailableCount() == 4);
}

TEST_CASE("every packet returns to the pool when the mailbox and the history are gone", "[client][packet]") {
    FramePacketPool pool;
    {
        TechEngine::PacketHistory history{pool};
        TechEngine::PacketMailbox mailbox{pool};
        for (std::uint64_t tick = 1; tick <= 3; tick++) {
            std::unique_ptr<FramePacket> packet = pool.acquire();
            REQUIRE(packet != nullptr);
            stamp(*packet, tick);
            mailbox.publish(std::move(packet));
            mailbox.deliverTo(history);
        }
        std::unique_ptr<FramePacket> waiting = pool.acquire();
        REQUIRE(waiting != nullptr);
        stamp(*waiting, 4);
        mailbox.publish(std::move(waiting));
        CHECK(pool.getAvailableCount() == 1);
    }
    CHECK(pool.getAvailableCount() == 4);
}
