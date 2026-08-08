#include "AssertCapture.hpp"

#include <TechEngine/core/events/EventRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

namespace {
    struct CollisionEnter {
        std::uint32_t entity;
        float impulse;
    };

    struct GamePaused {};

    struct DamageDealt {
        std::uint64_t source;
        std::int32_t amount;
    };

    struct TagSharer {
        std::uint32_t value;
    };

    struct SecondRegistryEvent {
        std::uint32_t value;
    };

    struct LocalByDefault {
        std::uint32_t value;
    };

    struct NeverRegistered {
        std::uint32_t value;
    };

    struct RejectedEvent {
        std::uint32_t value;
    };

    struct AfterRejection {
        std::uint32_t value;
    };

    struct EmptyTagEvent {
        std::uint32_t value;
    };

    struct AfterSeal {
        std::uint32_t value;
    };
}

TEST_CASE("registerEvent returns the tag's hash and fills the type slot", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");

    REQUIRE(id == TechEngine::EventTypeId{TechEngine::StringId{"TechEngine.CollisionEnter"}});
    REQUIRE(TechEngine::eventTypeId<CollisionEnter>() == id);
    REQUIRE(registry.typeCount() == 1);
}

TEST_CASE("dense stream indices are assigned in call order", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId paused = registry.registerEvent<GamePaused>("TechEngine.GamePaused");
    const TechEngine::EventTypeId damage = registry.registerEvent<DamageDealt>("TechEngine.DamageDealt");

    REQUIRE(registry.find(paused)->streamIndex == 0);
    REQUIRE(registry.find(damage)->streamIndex == 1);
}

TEST_CASE("the record carries the payload's layout and its wire flag", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<DamageDealt>("TechEngine.DamageDealt", TechEngine::EventWire::Replicated);
    const TechEngine::EventTypeRecord* record = registry.find(id);

    REQUIRE(record != nullptr);
    REQUIRE(record->size == sizeof(DamageDealt));
    REQUIRE(record->alignment == alignof(DamageDealt));
    REQUIRE(record->wire == TechEngine::EventWire::Replicated);
}

TEST_CASE("an event is Local unless the call says otherwise", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<LocalByDefault>("TechEngine.LocalByDefault");

    REQUIRE(registry.find(id)->wire == TechEngine::EventWire::Local);
}

// An event with no payload ("the game paused") is a legal event, not a degenerate one —
// sizeof is 1 because C++ has no zero-sized type, and the stream still carries occurrences.
TEST_CASE("an empty payload is a legal event", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<GamePaused>("TechEngine.GamePaused");
    const TechEngine::EventTypeRecord* record = registry.find(id);

    REQUIRE(record != nullptr);
    REQUIRE(record->size == sizeof(GamePaused));
    REQUIRE(record->alignment == alignof(GamePaused));
}

TEST_CASE("an unregistered type's slot is the invalid id", "[core][events]") {
    REQUIRE_FALSE(TechEngine::eventTypeId<NeverRegistered>().valid());
}

TEST_CASE("tagOf resolves a registered id and misses stay empty", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");

    REQUIRE(registry.tagOf(id) == "TechEngine.CollisionEnter");
    REQUIRE(registry.tagOf(TechEngine::EventTypeId{TechEngine::StringId{"TechEngine.NeverRegistered"}}).empty());
    REQUIRE(registry.find(TechEngine::EventTypeId{TechEngine::StringId{"TechEngine.NeverRegistered"}}) == nullptr);
}

// The invalid id is what a rejected registration hands back, so a caller that skips the
// valid() check lands here rather than on someone else's record.
TEST_CASE("the invalid id resolves to nothing", "[core][events]") {
    TechEngine::EventRegistry registry;

    registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");

    REQUIRE(registry.find(TechEngine::EventTypeId{}) == nullptr);
    REQUIRE(registry.tagOf(TechEngine::EventTypeId{}).empty());
}

TEST_CASE("a duplicate tag fires a check and leaves the registry unchanged", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;

    registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");
    const TechEngine::EventTypeId second = registry.registerEvent<TagSharer>("TechEngine.CollisionEnter");

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Check);
    REQUIRE_FALSE(second.valid());
    REQUIRE(registry.typeCount() == 1);
    REQUIRE_FALSE(TechEngine::eventTypeId<TagSharer>().valid());
}

// An empty tag hashes to the FNV offset basis, which is a perfectly valid id — so this
// cannot ride the id.valid() check; the guard is on the string.
TEST_CASE("an empty tag is rejected", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;

    const TechEngine::EventTypeId id = registry.registerEvent<EmptyTagEvent>("");

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Check);
    REQUIRE_FALSE(id.valid());
    REQUIRE(registry.typeCount() == 0);
    REQUIRE_FALSE(TechEngine::eventTypeId<EmptyTagEvent>().valid());
}

// A rejected registration must not burn a dense index: the indices are stream slots, and a
// hole here would mis-size every stream array T9 builds from this registry.
TEST_CASE("a rejected registration does not consume a stream index", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;

    registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");
    registry.registerEvent<RejectedEvent>("TechEngine.CollisionEnter");
    const TechEngine::EventTypeId accepted = registry.registerEvent<AfterRejection>("TechEngine.AfterRejection");

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(registry.typeCount() == 2);
    REQUIRE(registry.find(accepted)->streamIndex == 1);
}

// The streams are built from the records once, so a late registration would otherwise
// surface as a missing stream at the first publish, far from the call that caused it.
TEST_CASE("registration after the seal is rejected", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;

    registry.registerEvent<CollisionEnter>("TechEngine.CollisionEnter");
    registry.seal();
    const TechEngine::EventTypeId id = registry.registerEvent<AfterSeal>("TechEngine.AfterSeal");

    REQUIRE(registry.sealed());
    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Check);
    REQUIRE_FALSE(id.valid());
    REQUIRE(registry.typeCount() == 1);
    REQUIRE_FALSE(TechEngine::eventTypeId<AfterSeal>().valid());
}

// The slot holds the id, never the dense index — that is what lets two registries in one
// process disagree about stream layout while agreeing about identity.
TEST_CASE("the same type registered in a second registry keeps its id", "[core][events]") {
    TechEngine::EventRegistry first;
    TechEngine::EventRegistry second;

    second.registerEvent<GamePaused>("TechEngine.GamePaused");
    const TechEngine::EventTypeId fromFirst = first.registerEvent<SecondRegistryEvent>("TechEngine.SecondRegistryEvent");
    const TechEngine::EventTypeId fromSecond = second.registerEvent<SecondRegistryEvent>("TechEngine.SecondRegistryEvent");

    REQUIRE(fromFirst == fromSecond);
    REQUIRE(first.find(fromFirst)->streamIndex == 0);
    REQUIRE(second.find(fromSecond)->streamIndex == 1);
}