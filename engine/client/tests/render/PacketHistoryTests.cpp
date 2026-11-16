#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <render/FramePacketPool.hpp>
#include <render/FrameView.hpp>
#include <render/PacketHistory.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

using namespace std::chrono_literals;
using TechEngine::Clock;
using TechEngine::Entity;
using TechEngine::FramePacket;
using TechEngine::FramePacketPool;
using TechEngine::FrameView;
using TechEngine::Mat4;
using TechEngine::PacketHistory;
using TechEngine::PacketInstance;
using TechEngine::Vec4;

static std::unique_ptr<FramePacket> packetAt(FramePacketPool& pool, std::uint64_t tick, std::chrono::milliseconds time, float color, std::uint64_t sequence = 0) {
    std::unique_ptr<FramePacket> packet = pool.acquire();
    REQUIRE(packet != nullptr);
    packet->tick = tick;
    packet->tickTime = Clock::TimePoint{time};
    packet->fixedDeltaTime = 0.1;
    packet->timeline = 1;
    packet->sequence = sequence;
    packet->clearColor.fill(color);
    return packet;
}

static FrameView frameAt(const PacketHistory& history, std::chrono::milliseconds now, const TechEngine::InputState& input = {}) {
    FrameView view;
    history.prepareFrame(Clock::TimePoint{now}, input, view);
    return view;
}

static Mat4 translationX(float x) {
    Mat4 world = Mat4(1.0f);
    world[3] = Vec4(x, 0.0f, 0.0f, 1.0f);
    return world;
}

static void addInstance(FramePacket& packet, Entity entity, float x) {
    PacketInstance instance;
    instance.entity = entity;
    instance.world = translationX(x);
    packet.instances.push_back(instance);
}

TEST_CASE("render history interpolates actual timestamp gaps and holds endpoints", "[client][history]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    CHECK_FALSE(frameAt(history, 0ms).drawTriangle);
    history.acquire(packetAt(pool, 100, 1000ms, 0.0F));
    CHECK(history.alpha(Clock::TimePoint{1150ms}) == 1.0);
    history.acquire(packetAt(pool, 103, 1300ms, 1.0F));
    CHECK(history.alpha(Clock::TimePoint{1250ms}) == Catch::Approx(0.5));
    CHECK(frameAt(history, 1250ms).clearColor[0] == Catch::Approx(0.5));
    CHECK(history.alpha(Clock::TimePoint{1000ms}) == 0.0);
    CHECK(history.alpha(Clock::TimePoint{9000ms}) == 1.0);
    history.acquire(packetAt(pool, 103, 1300ms, 0.2F));
    history.acquire(packetAt(pool, 102, 1200ms, 0.2F));
    CHECK(frameAt(history, 1250ms).clearColor[0] == Catch::Approx(0.5));
    CHECK(pool.getAvailableCount() == 2);
}

TEST_CASE("render history resets on a timeline change and ignores stale generations", "[client][history]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    history.acquire(packetAt(pool, 100, 1000ms, 0.0F));
    history.acquire(packetAt(pool, 103, 1300ms, 1.0F));
    std::unique_ptr<FramePacket> reset = packetAt(pool, 0, 2000ms, 0.25F);
    reset->timeline = 2;
    history.acquire(std::move(reset));
    history.acquire(packetAt(pool, 999, 9000ms, 1.0F));
    CHECK(history.alpha(Clock::TimePoint{2100ms}) == 1.0);
    CHECK(frameAt(history, 2100ms).clearColor[0] == 0.25F);
    CHECK(pool.getAvailableCount() == 3);
}

TEST_CASE("latest cumulative look survives skipped reads without double application", "[client][history][input]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> first = packetAt(pool, 1, 1000ms, 0.0F);
    first->input.focusGeneration = 1;
    first->input.sequence = 10;
    first->input.lookX = 5.0;
    auto input = first->input;
    history.acquire(std::move(first));
    input.sequence = 12;
    input.lookX = 9.0;
    CHECK(frameAt(history, 1100ms, input).input.lookX == 9.0);
    CHECK(frameAt(history, 1100ms, input).input.lookX == 9.0);
    std::unique_ptr<FramePacket> second = packetAt(pool, 2, 1100ms, 0.0F);
    second->input = input;
    history.acquire(std::move(second));
    CHECK(frameAt(history, 1200ms, input).input.lookX == 9.0);
    input.focusGeneration++;
    input.sequence++;
    input.lookX = 0.0;
    CHECK(frameAt(history, 1200ms, input).input.lookX == 0.0);
}

TEST_CASE("an edit at the current tick replaces the current packet and keeps the previous one", "[client][history]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    history.acquire(packetAt(pool, 1, 1000ms, 0.0F, 1));
    history.acquire(packetAt(pool, 2, 1100ms, 1.0F, 2));
    REQUIRE(pool.getAvailableCount() == 2);

    history.acquire(packetAt(pool, 2, 1100ms, 0.5F, 3));
    CHECK(pool.getAvailableCount() == 2);
    const FrameView view = frameAt(history, 1150ms);
    REQUIRE(view.packet != nullptr);
    CHECK(view.packet->sequence == 3);
    CHECK(view.clearColor[0] == Catch::Approx(0.25));
}

TEST_CASE("a packet at the current tick with an older or equal sequence is dropped", "[client][history]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    history.acquire(packetAt(pool, 1, 1000ms, 0.0F, 1));
    history.acquire(packetAt(pool, 2, 1100ms, 1.0F, 3));

    history.acquire(packetAt(pool, 2, 1100ms, 0.5F, 3));
    history.acquire(packetAt(pool, 2, 1100ms, 0.5F, 2));
    CHECK(pool.getAvailableCount() == 2);
    const FrameView view = frameAt(history, 9000ms);
    REQUIRE(view.packet != nullptr);
    CHECK(view.packet->sequence == 3);
    CHECK(view.clearColor[0] == 1.0F);
}

TEST_CASE("every edit of a Stopped editor reaches the frame although the tick stays at zero", "[client][history]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    for (std::uint64_t sequence = 1; sequence <= 5; sequence++) {
        history.acquire(packetAt(pool, 0, 0ms, static_cast<float>(sequence), sequence));
        const FrameView view = frameAt(history, 500ms);
        REQUIRE(view.packet != nullptr);
        CHECK(view.packet->sequence == sequence);
        CHECK(view.clearColor[0] == static_cast<float>(sequence));
    }
    CHECK(pool.getAvailableCount() == 3);
}

TEST_CASE("before any packet the frame has nothing to draw", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    const FrameView view = frameAt(history, 1000ms);
    CHECK(view.packet == nullptr);
    CHECK(view.instanceWorlds.empty());
    CHECK_FALSE(view.hasCamera);
}

TEST_CASE("with one packet every instance draws at its own world", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> only = packetAt(pool, 1, 1000ms, 0.0F);
    addInstance(*only, Entity{1, 0}, 3.0f);
    addInstance(*only, Entity{2, 0}, 7.0f);
    history.acquire(std::move(only));

    const FrameView view = frameAt(history, 1050ms);
    REQUIRE(view.instanceWorlds.size() == 2);
    CHECK(view.instanceWorlds[0][3].x == 3.0f);
    CHECK(view.instanceWorlds[1][3].x == 7.0f);
}

TEST_CASE("an instance in both packets is interpolated, matched by entity rather than by position", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> previous = packetAt(pool, 1, 1000ms, 0.0F);
    addInstance(*previous, Entity{1, 0}, 0.0f);
    addInstance(*previous, Entity{2, 0}, 10.0f);
    history.acquire(std::move(previous));
    std::unique_ptr<FramePacket> current = packetAt(pool, 2, 1100ms, 0.0F);
    addInstance(*current, Entity{2, 0}, 20.0f);
    addInstance(*current, Entity{1, 0}, 10.0f);
    history.acquire(std::move(current));

    const FrameView view = frameAt(history, 1150ms);
    REQUIRE(view.instanceWorlds.size() == 2);
    CHECK(view.instanceWorlds[0][3].x == Catch::Approx(15.0f));
    CHECK(view.instanceWorlds[1][3].x == Catch::Approx(5.0f));
    CHECK(view.instanceWorlds[0][3].w == Catch::Approx(1.0f));
}

TEST_CASE("an instance that appears draws at its current world, and one that vanishes is not drawn", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> previous = packetAt(pool, 1, 1000ms, 0.0F);
    addInstance(*previous, Entity{1, 0}, 0.0f);
    addInstance(*previous, Entity{9, 0}, 50.0f);
    history.acquire(std::move(previous));
    std::unique_ptr<FramePacket> current = packetAt(pool, 2, 1100ms, 0.0F);
    addInstance(*current, Entity{3, 0}, 30.0f);
    addInstance(*current, Entity{1, 0}, 10.0f);
    history.acquire(std::move(current));

    const FrameView view = frameAt(history, 1150ms);
    REQUIRE(view.instanceWorlds.size() == 2);
    CHECK(view.instanceWorlds[0][3].x == 30.0f);
    CHECK(view.instanceWorlds[1][3].x == Catch::Approx(5.0f));
}

TEST_CASE("an entity reused with a new generation is a new instance, not the old one moved", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> previous = packetAt(pool, 1, 1000ms, 0.0F);
    addInstance(*previous, Entity{4, 0}, 0.0f);
    history.acquire(std::move(previous));
    std::unique_ptr<FramePacket> current = packetAt(pool, 2, 1100ms, 0.0F);
    addInstance(*current, Entity{4, 1}, 10.0f);
    history.acquire(std::move(current));

    const FrameView view = frameAt(history, 1150ms);
    REQUIRE(view.instanceWorlds.size() == 1);
    CHECK(view.instanceWorlds[0][3].x == 10.0f);
}

TEST_CASE("the frame view reuses its instance buffer when the instance count drops", "[client][history][instances]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> many = packetAt(pool, 1, 1000ms, 0.0F);
    for (std::uint32_t i = 0; i < 64; i++) {
        addInstance(*many, Entity{i, 0}, static_cast<float>(i));
    }
    history.acquire(std::move(many));
    FrameView view;
    history.prepareFrame(Clock::TimePoint{1000ms}, {}, view);
    REQUIRE(view.instanceWorlds.size() == 64);

    std::unique_ptr<FramePacket> few = packetAt(pool, 2, 1100ms, 0.0F);
    addInstance(*few, Entity{0, 0}, 0.0f);
    history.acquire(std::move(few));
    history.prepareFrame(Clock::TimePoint{9000ms}, {}, view);
    CHECK(view.instanceWorlds.size() == 1);
    CHECK(view.instanceWorlds.capacity() >= 64);
}

TEST_CASE("the camera's world is interpolated between the two packets", "[client][history][camera]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> previous = packetAt(pool, 1, 1000ms, 0.0F);
    previous->hasCamera = true;
    previous->camera.world = translationX(0.0f);
    history.acquire(std::move(previous));
    std::unique_ptr<FramePacket> current = packetAt(pool, 2, 1100ms, 0.0F);
    current->hasCamera = true;
    current->camera.world = translationX(10.0f);
    history.acquire(std::move(current));

    const FrameView view = frameAt(history, 1150ms);
    CHECK(view.hasCamera);
    CHECK(view.camera.world[3].x == Catch::Approx(5.0f));
}

TEST_CASE("a packet with no camera gives a frame with no camera", "[client][history][camera]") {
    FramePacketPool pool;
    PacketHistory history{pool};
    std::unique_ptr<FramePacket> previous = packetAt(pool, 1, 1000ms, 0.0F);
    previous->hasCamera = true;
    history.acquire(std::move(previous));
    history.acquire(packetAt(pool, 2, 1100ms, 0.0F));

    CHECK_FALSE(frameAt(history, 1150ms).hasCamera);
}
