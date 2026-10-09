#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/WindowError.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <system_error>
#include <thread>

static std::uint64_t renderedFrame(const TechEngine::Client& client) {
    TechEngine::RenderTiming timing;
    const bool active = client.renderTiming(timing);
    if (!active) {
        return 0;
    }
    return timing.frame;
}

TEST_CASE("Client starts GL and shuts down before destroying its window", "[client][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    TechEngine::EngineContext engine{files, jobs, clock};
    TechEngine::InputBuffer input{clock};
    TechEngine::Client client;
    TechEngine::RenderTiming timing;
    CHECK_FALSE(client.renderTiming(timing));
    REQUIRE(client.start(engine, input, 320, 240, "Client test") == std::error_code{});
    CHECK_FALSE(client.shouldClose());
    CHECK(client.start(engine, input, 320, 240, "Duplicate client start") == TechEngine::ClientError::AlreadyStarted);
    client.setTitle("Client test | FPS: 8 | TPS: 60");
    client.waitEvents(0.0);
    client.stop();
    CHECK(client.shouldClose());
    CHECK_FALSE(client.renderTiming(timing));
    client.stop();
}

TEST_CASE("Client cleans up a failed window creation and can retry", "[client][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    TechEngine::EngineContext engine{files, jobs, clock};
    TechEngine::InputBuffer input{clock};
    TechEngine::Client client;
    CHECK(client.start(engine, input, 0, 240, "Invalid window") == TechEngine::WindowError::InvalidSize);
    CHECK(client.shouldClose());
    client.waitEvents(0.0);
    client.setTitle("No window yet");
    client.stop();

    REQUIRE(client.start(engine, input, 320, 240, "Retry window") == std::error_code{});
    CHECK_FALSE(client.shouldClose());
    client.stop();
    CHECK(client.shouldClose());
}

TEST_CASE("client render pacing can toggle vsync without a simulation publication", "[client][window][pacing]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    TechEngine::EngineContext engine{files, jobs, clock};
    TechEngine::InputBuffer input{clock};
    TechEngine::Client client;
    client.setVSync(false);
    REQUIRE(client.start(engine, input, 320, 240, "Pacing test") == std::error_code{});
    const auto deadline = clock.now() + std::chrono::seconds{3};
    while (renderedFrame(client) < 3 && clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    TechEngine::RenderTiming before;
    REQUIRE(client.renderTiming(before));
    CHECK(before.frame >= 3);
    client.setVSync(true);
    while (renderedFrame(client) < before.frame + 2 && clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    CHECK(renderedFrame(client) >= before.frame + 2);
    client.stop();
    CHECK_FALSE(client.failed());
    TechEngine::RenderTiming stopped;
    CHECK_FALSE(client.renderTiming(stopped));
}
