#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStream.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

static constexpr std::string_view DAMAGE_TAG = "TechEngine.Damage";

struct Damage {
    std::uint32_t amount;
};

template<typename T>
static TechEngine::EventStream makeStream(std::string_view tag, std::size_t capacity) {
    TechEngine::EventRegistry registry;
    const TechEngine::EventTypeId id = registry.registerEvent<T>(tag);
    return TechEngine::EventStream{id, sizeof(T), alignof(T), capacity};
}

static std::vector<std::uint32_t> amountsOf(std::span<const Damage> events) {
    std::vector<std::uint32_t> amounts;
    for (const Damage& event: events) {
        amounts.push_back(event.amount);
    }
    return amounts;
}

TEST_CASE("a visible batch stays valid while the same type publishes and staging grows", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 2);

    stream.publish(Damage{1});
    stream.publish(Damage{2});
    stream.makeVisible(1);

    const std::span<const Damage> visible = stream.read<Damage>();
    std::vector<std::uint32_t> handled;
    for (const Damage& event: visible) {
        handled.push_back(event.amount);
        for (std::uint32_t i = 0; i < 32; i++) {
            stream.publish(Damage{event.amount * 100 + i});
        }
    }

    REQUIRE(handled == std::vector<std::uint32_t>{1, 2});
    REQUIRE(amountsOf(visible) == std::vector<std::uint32_t>{1, 2});
    REQUIRE(stream.visibleCount() == 2);
    REQUIRE(stream.stagedCount() == 64);

    stream.retire();
    stream.makeVisible(2);

    const std::vector<std::uint32_t> next = amountsOf(stream.read<Damage>());
    REQUIRE(next.size() == 64);
    REQUIRE(next.front() == 100);
    REQUIRE(next[31] == 131);
    REQUIRE(next[32] == 200);
    REQUIRE(next.back() == 231);
}

TEST_CASE("publishing stages; nothing is visible until the barrier", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{7});

    REQUIRE(stream.stagedCount() == 1);
    REQUIRE(stream.visibleCount() == 0);

    stream.makeVisible(1);

    REQUIRE(stream.stagedCount() == 0);
    REQUIRE(stream.visibleCount() == 1);
}

TEST_CASE("a visible batch stays until retire is asked", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{7});
    stream.makeVisible(1);

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{7});
    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{7});

    stream.publish(Damage{8});

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{7});

    stream.retire();

    REQUIRE(stream.read<Damage>().empty());
    REQUIRE(stream.visibleCount() == 0);
}

TEST_CASE("retiring keeps the events staged since the barrier", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(1);
    stream.publish(Damage{2});
    stream.publish(Damage{3});

    stream.retire();

    REQUIRE(stream.visibleCount() == 0);
    REQUIRE(stream.stagedCount() == 2);

    stream.makeVisible(2);

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{2, 3});
    REQUIRE(stream.stagedCount() == 0);
}

TEST_CASE("the first Tick's retire keeps its own publications", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{4});
    stream.retire();

    REQUIRE(stream.stagedCount() == 1);

    stream.makeVisible(1);

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{4});
}

TEST_CASE("a quiet Tick exposes an empty batch", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(1);

    stream.retire();
    stream.makeVisible(2);

    REQUIRE(stream.read<Damage>().empty());
    REQUIRE(stream.visibleCount() == 0);
    REQUIRE(stream.stagedCount() == 0);

    stream.publish(Damage{5});
    stream.retire();
    stream.makeVisible(3);

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{5});
}

TEST_CASE("consecutive Ticks each expose only their own batch", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 4);
    std::vector<std::uint32_t> previous;

    for (std::uint32_t tick = 1; tick <= 5; tick++) {
        REQUIRE(amountsOf(stream.read<Damage>()) == previous);

        previous.clear();
        for (std::uint32_t i = 0; i < tick; i++) {
            stream.publish(Damage{tick * 10 + i});
            previous.push_back(tick * 10 + i);
        }

        stream.retire();
        stream.makeVisible(tick);
    }

    REQUIRE(amountsOf(stream.read<Damage>()) == previous);
}

TEST_CASE("a batch keeps publisher order and FIFO within a publisher", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.publish(Damage{2});
    stream.publish(Damage{3});
    stream.publish(Damage{10});
    stream.publish(Damage{11});
    stream.makeVisible(1);

    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{1, 2, 3, 10, 11});
}

TEST_CASE("a batch reports the Tick that made it visible", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    REQUIRE(stream.visibleTick() == 0);

    stream.publish(Damage{1});
    stream.makeVisible(7);

    REQUIRE(stream.visibleTick() == 7);

    stream.retire();

    REQUIRE(stream.visibleTick() == 7);

    stream.makeVisible(8);

    REQUIRE(stream.visibleTick() == 8);
    REQUIRE(stream.read<Damage>().empty());
}

TEST_CASE("making a batch visible before retiring the last one is rejected and changes nothing", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(1);
    stream.publish(Damage{2});
    stream.makeVisible(2);

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Verify);
    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{1});
    REQUIRE(stream.stagedCount() == 1);
    REQUIRE(stream.visibleTick() == 1);

    stream.retire();
    stream.makeVisible(2);

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(amountsOf(stream.read<Damage>()) == std::vector<std::uint32_t>{2});
    REQUIRE(stream.visibleTick() == 2);
}

TEST_CASE("the buffer grows and keeps every event", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 2);

    for (std::uint32_t i = 0; i < 5; i++) {
        stream.publish(Damage{i});
    }

    REQUIRE(stream.capacity() >= 5);

    stream.makeVisible(1);

    const std::span<const Damage> all = stream.read<Damage>();

    REQUIRE(all.size() == 5);
    for (std::uint32_t i = 0; i < 5; i++) {
        REQUIRE(all[i].amount == i);
    }
}

TEST_CASE("a steady-state loop never regrows the buffers", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 64);

    stream.publish(Damage{0});
    stream.makeVisible(0);

    for (std::uint64_t tick = 1; tick <= 8; tick++) {
        for (std::uint32_t i = 0; i < 4; i++) {
            stream.publish(Damage{i});
        }
        stream.retire();
        stream.makeVisible(tick);
    }

    REQUIRE(stream.capacity() == 64);
    REQUIRE(stream.visibleCount() == 4);
}
