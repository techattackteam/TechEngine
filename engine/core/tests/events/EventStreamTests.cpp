#include <TechEngine/core/events/EventStream.hpp>

#include <TechEngine/core/events/EventRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <span>
#include <string_view>

static constexpr std::string_view DAMAGE_TAG = "TechEngine.Damage";

struct Damage {
    std::uint32_t amount;
};

// Registration is what writes the process-global type slot that publish/read check; the
// registry itself is not referenced afterwards, so a throwaway one is enough.
template<typename T>
static TechEngine::EventStream makeStream(std::string_view tag, std::size_t capacity) {
    TechEngine::EventRegistry registry;
    const TechEngine::EventTypeId id = registry.registerEvent<T>(tag);
    return TechEngine::EventStream{id, sizeof(T), alignof(T), capacity};
}

static std::size_t g_allocationCount = 0;

void* operator new(std::size_t size) {
    g_allocationCount++;
    void* memory = std::malloc(size);
    if (memory == nullptr) {
        throw std::bad_alloc{};
    }
    return memory;
}

void* operator new[](std::size_t size) {
    g_allocationCount++;
    void* memory = std::malloc(size);
    if (memory == nullptr) {
        throw std::bad_alloc{};
    }
    return memory;
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

TEST_CASE("publishing stages; nothing is visible until the barrier", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{7});

    REQUIRE(stream.stagedCount() == 1);
    REQUIRE(stream.visibleCount() == 0);

    stream.makeVisible(0, 0);

    REQUIRE(stream.stagedCount() == 0);
    REQUIRE(stream.visibleCount() == 1);
}

// ADR-014 §3 — the retire rule is an AND, and these three cases are the reason it is one.
TEST_CASE("retiring needs both a frame boundary and a tick", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{7});
    stream.makeVisible(0, 0);

    stream.retire(1, 0);
    REQUIRE(stream.visibleCount() == 1);

    stream.retire(0, 1);
    REQUIRE(stream.visibleCount() == 1);

    stream.retire(1, 1);
    REQUIRE(stream.visibleCount() == 0);
    REQUIRE(stream.retireHeadSequence() == 1);
}

// The classic fixed-timestep footgun: a run of frames fast enough to advance no tick must
// not age an event out before any fixed-domain reader has had a look.
TEST_CASE("events survive back-to-back frames that run no ticks", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{7});
    stream.makeVisible(0, 0);

    stream.retire(1, 0);
    stream.retire(2, 0);
    stream.retire(3, 0);
    REQUIRE(stream.visibleCount() == 1);

    stream.retire(4, 1);
    REQUIRE(stream.visibleCount() == 0);
}

TEST_CASE("only leading batches retire", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(0, 0);
    stream.publish(Damage{2});
    stream.makeVisible(0, 1);
    stream.publish(Damage{3});
    stream.makeVisible(0, 2);

    stream.retire(1, 1);

    REQUIRE(stream.retireHeadSequence() == 1);
    REQUIRE(stream.visibleCount() == 2);
}

TEST_CASE("a catch-up frame's sub-step batches all retire together", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(5, 10);
    stream.publish(Damage{2});
    stream.makeVisible(5, 11);
    stream.publish(Damage{3});
    stream.makeVisible(5, 12);

    stream.retire(6, 13);

    REQUIRE(stream.retireHeadSequence() == 3);
    REQUIRE(stream.visibleCount() == 0);
}

TEST_CASE("a cursor reads each retained event exactly once", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{11});
    stream.publish(Damage{22});
    stream.makeVisible(0, 0);

    TechEngine::EventCursor cursor;
    const std::span<const Damage> first = stream.read<Damage>(cursor);

    REQUIRE(first.size() == 2);
    REQUIRE(first[0].amount == 11);
    REQUIRE(first[1].amount == 22);

    const std::span<const Damage> second = stream.read<Damage>(cursor);

    REQUIRE(second.empty());
}

// ADR-014 §3's reader-independent retention: a reader that did not run inside the window
// misses silently rather than holding the buffer or dangling.
TEST_CASE("a lagging cursor clamps to the head and misses", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.publish(Damage{2});
    stream.makeVisible(0, 0);
    stream.retire(1, 1);

    stream.publish(Damage{3});
    stream.publish(Damage{4});
    stream.makeVisible(1, 1);

    TechEngine::EventCursor cursor;
    const std::span<const Damage> visible = stream.read<Damage>(cursor);

    REQUIRE(visible.size() == 2);
    REQUIRE(visible[0].amount == 3);
    REQUIRE(visible[1].amount == 4);
    REQUIRE(cursor.sequence == 4);
}

// The memmove case: retiring the leading batch shifts what is left down to index 0, and a
// wrong offset or length here reads as plausible garbage rather than a crash.
TEST_CASE("compaction preserves the retained payload", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 8);

    stream.publish(Damage{1});
    stream.makeVisible(0, 0);
    stream.publish(Damage{2});
    stream.publish(Damage{3});
    stream.makeVisible(0, 1);

    stream.retire(1, 1);

    REQUIRE(stream.retireHeadSequence() == 1);
    REQUIRE(stream.visibleCount() == 2);

    TechEngine::EventCursor cursor;
    const std::span<const Damage> retained = stream.read<Damage>(cursor);

    REQUIRE(retained.size() == 2);
    REQUIRE(retained[0].amount == 2);
    REQUIRE(retained[1].amount == 3);
}

TEST_CASE("the buffer grows and keeps every event", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 2);

    for (std::uint32_t i = 0; i < 5; i++) {
        stream.publish(Damage{i});
    }
    stream.makeVisible(0, 0);

    REQUIRE(stream.capacity() >= 5);

    TechEngine::EventCursor cursor;
    const std::span<const Damage> all = stream.read<Damage>(cursor);

    REQUIRE(all.size() == 5);
    for (std::uint32_t i = 0; i < 5; i++) {
        REQUIRE(all[i].amount == i);
    }
}

// No REQUIRE inside the loop: Catch2's expression decomposition is not guaranteed
// allocation-free, and it would be counted here.
TEST_CASE("steady-state publish, read and retire allocate nothing", "[core][events]") {
    TechEngine::EventStream stream = makeStream<Damage>(DAMAGE_TAG, 64);
    TechEngine::EventCursor cursor;

    stream.publish(Damage{0});
    stream.makeVisible(0, 0);
    stream.read<Damage>(cursor);
    stream.retire(1, 1);

    const std::size_t before = g_allocationCount;

    for (std::uint64_t frame = 1; frame <= 8; frame++) {
        stream.retire(frame, frame);
        for (std::uint32_t i = 0; i < 4; i++) {
            stream.publish(Damage{i});
        }
        stream.makeVisible(frame, frame);
        stream.read<Damage>(cursor);
    }

    REQUIRE(g_allocationCount == before);
    REQUIRE(stream.capacity() == 64);
}
