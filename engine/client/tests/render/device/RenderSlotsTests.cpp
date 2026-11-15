#include <render/device/RenderHandle.hpp>
#include <render/device/RenderSlots.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <set>
#include <type_traits>
#include <vector>

struct RenderSlotsTestPayload {
    int value = 0;
};

using TestSlots = TechEngine::RenderSlots<TechEngine::BufferHandle, RenderSlotsTestPayload>;

static_assert(!std::is_same_v<TechEngine::BufferHandle, TechEngine::TextureHandle>);
static_assert(!std::is_convertible_v<TechEngine::BufferHandle, TechEngine::PipelineHandle>);

TEST_CASE("A default render handle is null and resolves to nothing", "[client][render]") {
    TestSlots slots;
    const TechEngine::BufferHandle none{};
    CHECK_FALSE(none.valid());
    CHECK(slots.find(none) == nullptr);
    CHECK_FALSE(slots.destroy(none));
    CHECK(slots.size() == 0);
}

TEST_CASE("A created slot resolves to its payload until it is destroyed", "[client][render]") {
    TestSlots slots;
    const TechEngine::BufferHandle handle = slots.create(RenderSlotsTestPayload{7});
    REQUIRE(handle.valid());
    REQUIRE(slots.find(handle) != nullptr);
    CHECK(slots.find(handle)->value == 7);
    CHECK(slots.size() == 1);

    CHECK(slots.destroy(handle));
    CHECK(slots.find(handle) == nullptr);
    CHECK(slots.size() == 0);
    CHECK_FALSE(slots.destroy(handle));
    CHECK(slots.size() == 0);
}

TEST_CASE("A reused slot gets a new generation, and the old handle stays stale", "[client][render]") {
    TestSlots slots;
    const TechEngine::BufferHandle first = slots.create(RenderSlotsTestPayload{1});
    REQUIRE(slots.destroy(first));
    const TechEngine::BufferHandle second = slots.create(RenderSlotsTestPayload{2});

    CHECK(second.index == first.index);
    CHECK(second.generation != first.generation);
    CHECK(slots.find(first) == nullptr);
    REQUIRE(slots.find(second) != nullptr);
    CHECK(slots.find(second)->value == 2);
    CHECK_FALSE(slots.destroy(first));
    CHECK(slots.find(second) != nullptr);
}

TEST_CASE("Destroying one slot among many leaves the others resolving to their own payloads", "[client][render]") {
    TestSlots slots;
    std::vector<TechEngine::BufferHandle> handles;
    for (int i = 0; i < 64; i++) {
        handles.push_back(slots.create(RenderSlotsTestPayload{i}));
    }
    std::set<std::uint32_t> indices;
    for (const TechEngine::BufferHandle handle: handles) {
        indices.insert(handle.index);
    }
    CHECK(indices.size() == handles.size());

    REQUIRE(slots.destroy(handles[10]));
    REQUIRE(slots.destroy(handles[40]));
    CHECK(slots.size() == 62);
    for (int i = 0; i < 64; i++) {
        const RenderSlotsTestPayload* payload = slots.find(handles[static_cast<std::size_t>(i)]);
        if (i == 10 || i == 40) {
            CHECK(payload == nullptr);
        } else {
            REQUIRE(payload != nullptr);
            CHECK(payload->value == i);
        }
    }

    const TechEngine::BufferHandle refilled = slots.create(RenderSlotsTestPayload{100});
    CHECK((refilled.index == handles[10].index || refilled.index == handles[40].index));
    CHECK(slots.size() == 63);
}

TEST_CASE("A slot whose generation reaches the limit is retired instead of reused", "[client][render]") {
    TestSlots slots{1};
    const TechEngine::BufferHandle first = slots.create(RenderSlotsTestPayload{1});
    REQUIRE(slots.destroy(first));
    const TechEngine::BufferHandle second = slots.create(RenderSlotsTestPayload{2});
    REQUIRE(second.index == first.index);
    REQUIRE(second.generation == 1);
    REQUIRE(slots.destroy(second));

    const TechEngine::BufferHandle third = slots.create(RenderSlotsTestPayload{3});
    CHECK(third.index != first.index);
    CHECK(slots.find(first) == nullptr);
    CHECK(slots.find(second) == nullptr);
    REQUIRE(slots.find(third) != nullptr);
    CHECK(slots.find(third)->value == 3);
}

TEST_CASE("destroyAll visits each live payload once and leaves every handle stale", "[client][render]") {
    TestSlots slots;
    const TechEngine::BufferHandle a = slots.create(RenderSlotsTestPayload{1});
    const TechEngine::BufferHandle b = slots.create(RenderSlotsTestPayload{2});
    const TechEngine::BufferHandle c = slots.create(RenderSlotsTestPayload{4});
    REQUIRE(slots.destroy(b));

    int visits = 0;
    int sum = 0;
    slots.destroyAll([&](const RenderSlotsTestPayload& payload) {
        visits++;
        sum += payload.value;
    });
    CHECK(visits == 2);
    CHECK(sum == 5);
    CHECK(slots.size() == 0);
    CHECK(slots.find(a) == nullptr);
    CHECK(slots.find(c) == nullptr);

    int secondVisits = 0;
    slots.destroyAll([&](const RenderSlotsTestPayload&) {
        secondVisits++;
    });
    CHECK(secondVisits == 0);

    const TechEngine::BufferHandle after = slots.create(RenderSlotsTestPayload{8});
    REQUIRE(slots.find(after) != nullptr);
    CHECK(after != a);
    CHECK(after != c);
}
