#include <TechEngine/client/Client.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Client starts GL and shuts down before destroying its window", "[client][window]") {
    TechEngine::Client client;
    REQUIRE(client.start(320, 240, "Client test"));
    CHECK_FALSE(client.shouldClose());
    CHECK_FALSE(client.start(320, 240, "Duplicate client start"));
    client.pollEvents();
    client.stop();
    CHECK(client.shouldClose());
    client.stop();
}
