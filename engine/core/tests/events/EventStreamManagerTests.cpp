#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStreamManager.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <span>
#include <string_view>

static constexpr std::string_view ALPHA_TAG = "TechEngine.StreamsAlpha";
static constexpr std::string_view BETA_TAG = "TechEngine.StreamsBeta";

namespace {
    struct StreamsAlpha {
        std::uint32_t amount;
    };

    struct StreamsBeta {
        std::uint64_t source;
    };

    struct StreamsUnregistered {
        std::uint32_t amount;
    };
}

TEST_CASE("the container builds one stream per registered type", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    registry.registerEvent<StreamsBeta>(BETA_TAG);

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
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);

    REQUIRE_FALSE(registry.sealed());

    const TechEngine::EventStreamManager streams{registry};

    REQUIRE(registry.sealed());
}

TEST_CASE("publish and read reach the type's own stream", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    registry.registerEvent<StreamsBeta>(BETA_TAG);
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{7});
    streams.publish(StreamsBeta{99});
    streams.makeVisible(0, 0);

    TechEngine::EventCursor alphaCursor;
    TechEngine::EventCursor betaCursor;
    const std::span<const StreamsAlpha> alpha = streams.read<StreamsAlpha>(alphaCursor);
    const std::span<const StreamsBeta> beta = streams.read<StreamsBeta>(betaCursor);

    REQUIRE(alpha.size() == 1);
    REQUIRE(alpha[0].amount == 7);
    REQUIRE(beta.size() == 1);
    REQUIRE(beta[0].source == 99);
}

TEST_CASE("the barrier reaches every stream at once", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    registry.registerEvent<StreamsBeta>(BETA_TAG);
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{1});
    streams.publish(StreamsBeta{2});

    TechEngine::EventCursor alphaCursor;
    TechEngine::EventCursor betaCursor;

    REQUIRE(streams.read<StreamsAlpha>(alphaCursor).empty());
    REQUIRE(streams.read<StreamsBeta>(betaCursor).empty());

    streams.makeVisible(0, 0);

    REQUIRE(streams.read<StreamsAlpha>(alphaCursor).size() == 1);
    REQUIRE(streams.read<StreamsBeta>(betaCursor).size() == 1);
}

TEST_CASE("retiring reaches every stream at once", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    registry.registerEvent<StreamsBeta>(BETA_TAG);
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsAlpha{1});
    streams.publish(StreamsBeta{2});
    streams.makeVisible(0, 0);

    streams.retire(1, 1);

    TechEngine::EventCursor alphaCursor;
    TechEngine::EventCursor betaCursor;

    REQUIRE(streams.read<StreamsAlpha>(alphaCursor).empty());
    REQUIRE(streams.read<StreamsBeta>(betaCursor).empty());
}

// The miss is always-on and survivable: a handler that declines to abort must land on a
// defined path, not index the stream vector with a record that was never found.
TEST_CASE("a type with no stream is rejected and changes nothing", "[core][events]") {
    const TechEngineTests::AssertHandlerGuard guard;
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    TechEngine::EventStreamManager streams{registry};

    streams.publish(StreamsUnregistered{7});

    TechEngine::EventCursor cursor;
    const std::span<const StreamsUnregistered> missing = streams.read<StreamsUnregistered>(cursor);

    REQUIRE(TechEngineTests::g_fired.size() == 2);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Verify);
    REQUIRE(missing.empty());
    REQUIRE(cursor.sequence == 0);
    REQUIRE(streams.streamCount() == 1);
}

// The frame shape the driver runs: publish inside a fixed sub-step, read at the tail. The
// second frame is what proves the cursor parked rather than replaying the batch.
TEST_CASE("an event published in a sub-step is read once at the frame tail", "[core][events]") {
    TechEngine::EventRegistry registry;
    registry.registerEvent<StreamsAlpha>(ALPHA_TAG);
    TechEngine::EventStreamManager streams{registry};
    TechEngine::EventCursor cursor;

    streams.publish(StreamsAlpha{5});
    streams.makeVisible(1, 1);
    streams.makeVisible(1, 1);

    REQUIRE(streams.read<StreamsAlpha>(cursor).size() == 1);

    streams.retire(1, 1);
    streams.makeVisible(2, 2);

    REQUIRE(streams.read<StreamsAlpha>(cursor).empty());
}
