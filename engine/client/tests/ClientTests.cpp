#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Client starts GL and shuts down before destroying its window", "[client][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Client client;
    CHECK(client.renderFramesPerSecond() == 0.0);
    REQUIRE(client.start(jobs, 320, 240, "Client test"));
    CHECK_FALSE(client.shouldClose());
    CHECK_FALSE(client.start(jobs, 320, 240, "Duplicate client start"));
    client.setTitle("Client test | FPS: 8 | TPS: 60");
    client.pollEvents();
    client.stop();
    CHECK(client.shouldClose());
    CHECK(client.renderFramesPerSecond() == 0.0);
    client.stop();
}

TEST_CASE("Client cleans up a failed window creation and can retry", "[client][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Client client;
    CHECK_FALSE(client.start(jobs, 0, 240, "Invalid window"));
    CHECK(client.shouldClose());
    client.pollEvents();
    client.setTitle("No window yet");
    client.stop();

    REQUIRE(client.start(jobs, 320, 240, "Retry window"));
    CHECK_FALSE(client.shouldClose());
    client.stop();
    CHECK(client.shouldClose());
}
