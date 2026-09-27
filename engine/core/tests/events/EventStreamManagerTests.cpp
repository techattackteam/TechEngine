#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStreamManager.hpp>
#include <TechEngine/core/events/EventTypeId.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace {
    struct StreamsAlpha {
        static constexpr std::string_view tag = "TechEngine.StreamsAlpha";

        std::uint32_t amount;
    };

    struct StreamsBeta {
        static constexpr std::string_view tag = "TechEngine.StreamsBeta";

        std::uint64_t source;
    };

    struct StreamsUnregistered {
        std::uint32_t amount;
    };
}

TEST_CASE("the container builds one stream per registered type", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    registry.registerEvent<StreamsBeta>();

    const TechEngine::EventStreamManager streams{registry};

    REQUIRE(streams.streamCount() == 2);
}

TEST_CASE("an empty registry builds no streams", "[core][events]") {
    TechEngine::EventRegistry registry;

    const TechEngine::EventStreamManager streams{registry};

    REQUIRE(streams.streamCount() == 0);
}

TEST_CASE("building the streams seals the registry", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();

    REQUIRE_FALSE(registry.sealed());

    const TechEngine::EventStreamManager streams{registry};

    REQUIRE(registry.sealed());
}

TEST_CASE("publish and read reach the type's own stream", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    registry.registerEvent<StreamsBeta>();
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{7});
    streams.publish(StreamsBeta{99});
    streams.makeVisible(1);

    const std::span<const StreamsAlpha> alpha = streams.read<StreamsAlpha>();
    const std::span<const StreamsBeta> beta = streams.read<StreamsBeta>();

    REQUIRE(alpha.size() == 1);
    REQUIRE(alpha[0].amount == 7);
    REQUIRE(beta.size() == 1);
    REQUIRE(beta[0].source == 99);
}

TEST_CASE("the barrier reaches every stream at once", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    registry.registerEvent<StreamsBeta>();
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{1});
    streams.publish(StreamsBeta{2});

    REQUIRE(streams.read<StreamsAlpha>().empty());
    REQUIRE(streams.read<StreamsBeta>().empty());

    streams.makeVisible(1);

    REQUIRE(streams.read<StreamsAlpha>().size() == 1);
    REQUIRE(streams.read<StreamsBeta>().size() == 1);
}

TEST_CASE("retiring reaches every stream at once", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    registry.registerEvent<StreamsBeta>();
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{1});
    streams.publish(StreamsBeta{2});
    streams.makeVisible(1);

    streams.retire();

    REQUIRE(streams.read<StreamsAlpha>().empty());
    REQUIRE(streams.read<StreamsBeta>().empty());
}

TEST_CASE("reading bytes by type id sees exactly the typed visible batch", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    registry.registerEvent<StreamsBeta>();
    TechEngine::EventStreamManager streams{registry};

    REQUIRE(streams.readBytes(TechEngine::eventTypeId<StreamsAlpha>()).empty());

    streams.publish(StreamsAlpha{7});
    streams.publish(StreamsAlpha{9});
    streams.publish(StreamsBeta{99});

    REQUIRE(streams.readBytes(TechEngine::eventTypeId<StreamsAlpha>()).empty());

    streams.makeVisible(1);

    const std::span<const StreamsAlpha> typed = streams.read<StreamsAlpha>();
    const std::span<const std::byte> bytes = streams.readBytes(TechEngine::eventTypeId<StreamsAlpha>());

    REQUIRE(bytes.size() == 2 * sizeof(StreamsAlpha));
    REQUIRE(static_cast<const void*>(bytes.data()) == static_cast<const void*>(typed.data()));
    REQUIRE(streams.readBytes(TechEngine::eventTypeId<StreamsBeta>()).size() == sizeof(StreamsBeta));

    streams.retire();

    REQUIRE(streams.readBytes(TechEngine::eventTypeId<StreamsAlpha>()).empty());
}

TEST_CASE("reading bytes for a type with no stream is rejected and returns nothing", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    TechEngine::EventStreamManager streams{registry};

    const std::span<const std::byte> missing = streams.readBytes(TechEngine::eventTypeId<StreamsUnregistered>());

    REQUIRE(TechEngineTests::g_fired == std::vector<TechEngine::AssertKind>{TechEngine::AssertKind::Verify});
    REQUIRE(missing.empty());
}

// The miss is always-on and survivable: a handler that declines to abort must land on a
// defined path, not index the stream vector with a record that was never found.
TEST_CASE("a type with no stream is rejected and changes nothing", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>();
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsUnregistered{7});

    const std::span<const StreamsUnregistered> missing = streams.read<StreamsUnregistered>();

    REQUIRE(TechEngineTests::g_fired.size() == 2);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Verify);
    REQUIRE(missing.empty());
    REQUIRE(streams.streamCount() == 1);
}
