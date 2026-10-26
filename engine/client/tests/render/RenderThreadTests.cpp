#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/RecordingImGuiHook.hpp>
#include <render/RenderThread.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

TEST_CASE("Render thread loads GL with its context current", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render startup test") == std::error_code{});
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(jobs, clock, window, input));
    CHECK(GLAD_GL_VERSION_4_5 != 0);
    CHECK_FALSE(renderer.start(jobs, clock, window, input));
    const std::vector<TechEngine::ThreadInfo> registered = jobs.registeredThreads();
    REQUIRE(registered.size() == 2);
    const auto render = std::ranges::find(registered, "TERender", &TechEngine::ThreadInfo::name);
    REQUIRE(render != registered.end());
    CHECK(render->role == TechEngine::ThreadRole::Dedicated);
    CHECK(render->id != std::this_thread::get_id());
    jobs.shutdown();
    REQUIRE(jobs.registeredThreads().size() == 1);
    CHECK(jobs.registeredThreads().front().name == "TERender");
    renderer.stop();
    CHECK(jobs.registeredThreads().empty());

    bool queriedVersion = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            queriedVersion = glGetString(GL_VERSION) != nullptr;
            window.releaseContext();
        });
    }
    CHECK(queriedVersion);
}

TEST_CASE("Render thread releases its context and joins before window destruction", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render shutdown test") == std::error_code{});
    {
        TechEngine::RenderThread renderer;
        REQUIRE(renderer.start(jobs, clock, window, input));
        renderer.stop();
        renderer.stop();
        REQUIRE(renderer.start(jobs, clock, window, input));
    }
    window.close();
    CHECK(window.shouldClose());
}

TEST_CASE("Render thread joins after failed startup and can retry", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    TechEngine::RenderThread renderer;
    CHECK_FALSE(renderer.start(jobs, clock, window, input));
    REQUIRE(jobs.registeredThreads().size() == 1);
    CHECK(jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    REQUIRE(window.open(320, 240, "Startup retry test") == std::error_code{});
    REQUIRE(renderer.start(jobs, clock, window, input));
    renderer.stop();
    REQUIRE(jobs.registeredThreads().size() == 1);
    CHECK(jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    window.close();
}

TEST_CASE("Render thread starts, draws and stops its ImGui hook on itself in that order", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "ImGui hook order test") == std::error_code{});
    TechEngineTests::RecordingImGuiHook hook;
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(jobs, clock, window, input, {}, &hook));
    const std::vector<TechEngineTests::ImGuiHookCall> atReady = hook.calls();
    REQUIRE_FALSE(atReady.empty());
    CHECK(atReady.front().name == "start");
    REQUIRE(hook.waitForDraws(3));
    renderer.stop();
    renderer.stop();

    const std::vector<TechEngineTests::ImGuiHookCall> calls = hook.calls();
    REQUIRE(calls.size() >= 5);
    CHECK(calls.front().name == "start");
    CHECK(calls.back().name == "stop");
    for (std::size_t i = 1; i + 1 < calls.size(); i++) {
        CHECK(calls[i].name == "draw");
    }
    const std::thread::id renderThread = calls.front().thread;
    CHECK(renderThread != std::this_thread::get_id());
    for (const TechEngineTests::ImGuiHookCall& call: calls) {
        CHECK(call.thread == renderThread);
        CHECK(call.contextCurrent);
    }
}

TEST_CASE("Render thread runs without an ImGui hook and calls a later one only in its own session", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "ImGui hook session test") == std::error_code{});
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(jobs, clock, window, input));
    renderer.stop();

    TechEngineTests::RecordingImGuiHook hook;
    REQUIRE(renderer.start(jobs, clock, window, input, {}, &hook));
    REQUIRE(hook.waitForDraws(1));
    renderer.stop();
    REQUIRE(renderer.start(jobs, clock, window, input));
    renderer.stop();

    const std::vector<TechEngineTests::ImGuiHookCall> calls = hook.calls();
    REQUIRE(calls.size() >= 3);
    CHECK(std::ranges::count(calls, std::string{"start"}, &TechEngineTests::ImGuiHookCall::name) == 1);
    CHECK(std::ranges::count(calls, std::string{"stop"}, &TechEngineTests::ImGuiHookCall::name) == 1);
    CHECK(calls.back().name == "stop");
}

TEST_CASE("Render thread stops its ImGui hook after a failed frame", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "ImGui hook failure test") == std::error_code{});
    TechEngineTests::RecordingImGuiHook hook{true};
    std::atomic<bool> failed = false;
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(
        jobs,
        clock,
        window,
        input,
        [&] {
            failed.store(true);
        },
        &hook));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (!failed.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    REQUIRE(failed.load());
    renderer.stop();

    CHECK(renderer.completion().status == TechEngine::ThreadCompletionStatus::Failed);
    const std::vector<TechEngineTests::ImGuiHookCall> calls = hook.calls();
    REQUIRE(calls.size() == 3);
    CHECK(calls[0].name == "start");
    CHECK(calls[1].name == "draw");
    CHECK(calls[2].name == "stop");
    CHECK(calls[2].thread == calls[0].thread);
    CHECK(calls[2].contextCurrent);
}
