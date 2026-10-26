#include <TechEngine/client/Client.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/WindowError.hpp>
#include <TechEngine/testing/EngineServices.hpp>

#include <render/RecordingImGuiHook.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <system_error>
#include <thread>
#include <vector>

static std::uint64_t renderedFrame(const TechEngine::Client& client) {
    TechEngine::RenderTiming timing;
    const bool active = client.renderTiming(timing);
    if (!active) {
        return 0;
    }
    return timing.frame;
}

TEST_CASE("Client starts GL and shuts down before destroying its window", "[client][window]") {
    TechEngineTests::EngineServices services;
    TechEngine::InputBuffer input{services.clock};
    TechEngine::Client client;
    TechEngine::RenderTiming timing;
    CHECK_FALSE(client.renderTiming(timing));
    REQUIRE(client.start(services.engine, input, 320, 240, "Client test") == std::error_code{});
    CHECK_FALSE(client.shouldClose());
    CHECK(client.start(services.engine, input, 320, 240, "Duplicate client start") == TechEngine::ClientError::AlreadyStarted);
    client.setTitle("Client test | FPS: 8 | TPS: 60");
    client.waitEvents(0.0);
    client.stop();
    CHECK(client.shouldClose());
    CHECK_FALSE(client.renderTiming(timing));
    client.stop();
}

TEST_CASE("Client cleans up a failed window creation and can retry", "[client][window]") {
    TechEngineTests::EngineServices services;
    TechEngine::InputBuffer input{services.clock};
    TechEngine::Client client;
    CHECK(client.start(services.engine, input, 0, 240, "Invalid window") == TechEngine::WindowError::InvalidSize);
    CHECK(client.shouldClose());
    client.waitEvents(0.0);
    client.setTitle("No window yet");
    client.stop();

    REQUIRE(client.start(services.engine, input, 320, 240, "Retry window") == std::error_code{});
    CHECK_FALSE(client.shouldClose());
    client.stop();
    CHECK(client.shouldClose());
}

TEST_CASE("client render pacing can toggle vsync without a simulation publication", "[client][window][pacing]") {
    TechEngineTests::EngineServices services;
    TechEngine::InputBuffer input{services.clock};
    TechEngine::Client client;
    client.setVSync(false);
    REQUIRE(client.start(services.engine, input, 320, 240, "Pacing test") == std::error_code{});
    const auto deadline = services.clock.now() + std::chrono::seconds{3};
    while (renderedFrame(client) < 3 && services.clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    TechEngine::RenderTiming before;
    REQUIRE(client.renderTiming(before));
    CHECK(before.frame >= 3);
    client.setVSync(true);
    while (renderedFrame(client) < before.frame + 2 && services.clock.now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    CHECK(renderedFrame(client) >= before.frame + 2);
    client.stop();
    CHECK_FALSE(client.failed());
    TechEngine::RenderTiming stopped;
    CHECK_FALSE(client.renderTiming(stopped));
}

TEST_CASE("Client hands its ImGui hook to the render thread and exposes its window to main only while started", "[client][window]") {
    TechEngineTests::EngineServices services;
    TechEngine::InputBuffer input{services.clock};
    TechEngineTests::RecordingImGuiHook hook;
    TechEngine::Client client;
    CHECK(client.nativeWindow() == nullptr);
    REQUIRE(client.start(services.engine, input, 320, 240, "ImGui hook client test", {}, &hook) == std::error_code{});
    CHECK(client.nativeWindow() != nullptr);
    REQUIRE(hook.waitForDraws(1));
    client.stop();
    CHECK(client.nativeWindow() == nullptr);
    CHECK_FALSE(client.failed());

    const std::vector<TechEngineTests::ImGuiHookCall> calls = hook.calls();
    REQUIRE(calls.size() >= 3);
    CHECK(calls.front().name == "start");
    CHECK(calls.back().name == "stop");
    CHECK(calls.front().thread != std::this_thread::get_id());
}
