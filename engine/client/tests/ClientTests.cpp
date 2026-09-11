#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

TEST_CASE("Client starts GL and shuts down before destroying its window", "[client][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    TechEngine::EngineContext engine{files, jobs, clock};
    TechEngine::InputBuffer input{clock};
    TechEngine::Client client;
    CHECK_FALSE(client.renderTiming().has_value());
    REQUIRE(client.start(engine, input, 320, 240, "Client test"));
    CHECK_FALSE(client.shouldClose());
    CHECK_FALSE(client.start(engine, input, 320, 240, "Duplicate client start"));
    client.setTitle("Client test | FPS: 8 | TPS: 60");
    client.waitEvents(0.0);
    client.stop();
    CHECK(client.shouldClose());
    CHECK_FALSE(client.renderTiming().has_value());
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
    CHECK_FALSE(client.start(engine, input, 0, 240, "Invalid window"));
    CHECK(client.shouldClose());
    client.waitEvents(0.0);
    client.setTitle("No window yet");
    client.stop();

    REQUIRE(client.start(engine, input, 320, 240, "Retry window"));
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
    REQUIRE(client.start(engine, input, 320, 240, "Pacing test"));
    const auto deadline = clock.now() + std::chrono::seconds{3};
    while (client.renderTiming()->frame < 3 && clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const auto before = client.renderTiming();
    REQUIRE(before);
    CHECK(before->frame >= 3);
    client.setVSync(true);
    while (client.renderTiming()->frame < before->frame + 2 && clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    CHECK(client.renderTiming()->frame >= before->frame + 2);
    client.stop();
    CHECK_FALSE(client.failed());
    CHECK_FALSE(client.renderTiming());
}
