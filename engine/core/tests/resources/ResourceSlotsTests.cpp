#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <resources/ResourceSlots.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

using TechEngine::ResourceEntry;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceSlots;

TEST_CASE("a default slot id is null and finds nothing", "[core][resources][slots]") {
    const TechEngineTests::FatalAssertGuard guard;
    const ResourceSlots slots;
    const ResourceSlotId slot;

    REQUIRE_FALSE(slot.valid());
    REQUIRE(slots.find(slot) == nullptr);
    REQUIRE(slots.size() == 0);
}

TEST_CASE("a created slot is found by its own id", "[core][resources][slots]") {
    const TechEngineTests::FatalAssertGuard guard;
    ResourceSlots slots;

    const ResourceSlotId slot = slots.create();

    REQUIRE(slot.valid());
    REQUIRE(slots.find(slot) != nullptr);
    REQUIRE(slots.size() == 1);
}

TEST_CASE("a destroyed slot is reused with an advanced generation", "[core][resources][slots]") {
    const TechEngineTests::FatalAssertGuard guard;
    ResourceSlots slots;
    const ResourceSlotId first = slots.create();
    REQUIRE(slots.destroy(first));

    const ResourceSlotId reused = slots.create();

    REQUIRE(reused.index == first.index);
    REQUIRE(reused.generation == first.generation + 1);
    REQUIRE(slots.find(first) == nullptr);
    REQUIRE(slots.find(reused) != nullptr);
    REQUIRE(slots.size() == 1);
}

TEST_CASE("destroying a slot twice fails the second time", "[core][resources][slots]") {
    const TechEngineTests::FatalAssertGuard guard;
    ResourceSlots slots;
    const ResourceSlotId slot = slots.create();

    REQUIRE(slots.destroy(slot));
    REQUIRE_FALSE(slots.destroy(slot));
    REQUIRE(slots.size() == 0);
}

TEST_CASE("an entry pointer stays valid while the blocks grow around it", "[core][resources][slots]") {
    const TechEngineTests::FatalAssertGuard guard;
    ResourceSlots slots;

    const ResourceSlotId first = slots.create();
    ResourceEntry* entry = slots.find(first);
    REQUIRE(entry != nullptr);
    entry->id = ResourceId(0x1234);

    std::vector<ResourceSlotId> later;
    for (std::uint32_t i = 0; i < ResourceSlots::ENTRIES_PER_BLOCK * 3; i++) {
        later.push_back(slots.create());
    }

    REQUIRE(slots.find(first) == entry);
    REQUIRE(entry->id == ResourceId(0x1234));
    REQUIRE(slots.size() == ResourceSlots::ENTRIES_PER_BLOCK * 3 + 1);
    for (const ResourceSlotId slot: later) {
        REQUIRE(slots.find(slot) != nullptr);
    }
}
