#include <TechEngine/client/render/ImGuiHook.hpp>
#include <TechEngine/client/render/RenderView.hpp>
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
#include <cstdint>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

class RenderThreadFixture {
public:
    TechEngine::JobSystem jobs{1};
    TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngineTests::WindowTerminateGuard scope;
    TechEngine::Window window;

    RenderThreadFixture() {
        REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    }
};

TEST_CASE("Render thread loads GL with its context current", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "Render startup test") == std::error_code{});
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    CHECK(GLAD_GL_VERSION_4_6 != 0);
    CHECK(GLAD_GL_ARB_bindless_texture != 0);
    CHECK_FALSE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    const std::vector<TechEngine::ThreadInfo> registered = fixture.jobs.registeredThreads();
    REQUIRE(registered.size() == 2);
    const auto render = std::ranges::find(registered, "TERender", &TechEngine::ThreadInfo::name);
    REQUIRE(render != registered.end());
    CHECK(render->role == TechEngine::ThreadRole::Dedicated);
    CHECK(render->id != std::this_thread::get_id());
    fixture.jobs.shutdown();
    REQUIRE(fixture.jobs.registeredThreads().size() == 1);
    CHECK(fixture.jobs.registeredThreads().front().name == "TERender");
    renderer.stop();
    CHECK(fixture.jobs.registeredThreads().empty());

    bool queriedVersion = false;
    {
        std::jthread worker([&] {
            fixture.window.makeContextCurrent();
            queriedVersion = glGetString(GL_VERSION) != nullptr;
            fixture.window.releaseContext();
        });
    }
    CHECK(queriedVersion);
}

TEST_CASE("Render thread releases its context and joins before window destruction", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "Render shutdown test") == std::error_code{});
    {
        TechEngine::RenderThread renderer;
        REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
        renderer.stop();
        renderer.stop();
        REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    }
    fixture.window.close();
    CHECK(fixture.window.shouldClose());
}

TEST_CASE("Render thread joins after failed startup and can retry", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    TechEngine::RenderThread renderer;
    CHECK_FALSE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    REQUIRE(fixture.jobs.registeredThreads().size() == 1);
    CHECK(fixture.jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    REQUIRE(fixture.window.open(320, 240, "Startup retry test") == std::error_code{});
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    renderer.stop();
    REQUIRE(fixture.jobs.registeredThreads().size() == 1);
    CHECK(fixture.jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    fixture.window.close();
}

TEST_CASE("Render thread starts, draws and stops its ImGui hook on itself in that order", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "ImGui hook order test") == std::error_code{});
    TechEngineTests::RecordingImGuiHook hook;
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input, {}, &hook));
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

TEST_CASE("Render thread runs without an ImGui hook and calls a later one only in its own session", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "ImGui hook session test") == std::error_code{});
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    renderer.stop();

    TechEngineTests::RecordingImGuiHook hook;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input, {}, &hook));
    REQUIRE(hook.waitForDraws(1));
    renderer.stop();
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input));
    renderer.stop();

    const std::vector<TechEngineTests::ImGuiHookCall> calls = hook.calls();
    REQUIRE(calls.size() >= 3);
    CHECK(std::ranges::count(calls, std::string{"start"}, &TechEngineTests::ImGuiHookCall::name) == 1);
    CHECK(std::ranges::count(calls, std::string{"stop"}, &TechEngineTests::ImGuiHookCall::name) == 1);
    CHECK(calls.back().name == "stop");
}

TEST_CASE("Render thread stops its ImGui hook after a failed frame", "[client][render][window][gpu]") {
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "ImGui hook failure test") == std::error_code{});
    TechEngineTests::RecordingImGuiHook hook{true};
    std::atomic<bool> failed = false;
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(
        fixture.jobs,
        fixture.clock,
        fixture.window,
        fixture.input,
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

static std::vector<TechEngine::ImGuiViewTexture> texturesAfterTwoMoreDraws(const TechEngineTests::RecordingImGuiHook& hook) {
    // A frame already running may have read the views before the change, so the second draw after it is the first that must see it.
    const std::size_t seen = hook.calls().size();
    if (!hook.waitForDraws(seen + 2)) {
        return {};
    }
    return hook.calls().back().viewTextures;
}

TEST_CASE("Render thread hands its ImGui hook one texture per sized view, and starts each session with none", "[client][render][view][window][gpu]") {
    constexpr std::uint64_t SCENE_TOKEN = TechEngine::VIEW_TEXTURE_TOKEN_BIT | 1;
    constexpr std::uint64_t GAME_TOKEN = TechEngine::VIEW_TEXTURE_TOKEN_BIT | 2;
    RenderThreadFixture fixture;
    REQUIRE(fixture.window.open(320, 240, "ImGui view texture test") == std::error_code{});
    TechEngineTests::RecordingImGuiHook hook;
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input, {}, &hook));
    REQUIRE(hook.waitForDraws(2));
    const std::vector<TechEngine::ImGuiViewTexture> beforeViews = hook.calls().back().viewTextures;

    TechEngine::ViewRequest scene;
    scene.token = SCENE_TOKEN;
    scene.width = 64;
    scene.height = 48;
    scene.visible = true;
    TechEngine::ViewRequest game = scene;
    game.token = GAME_TOKEN;
    game.width = 32;
    game.height = 32;
    const std::vector<TechEngine::ViewRequest> both{scene, game};
    renderer.setViews(both);
    const std::vector<TechEngine::ImGuiViewTexture> withBoth = texturesAfterTwoMoreDraws(hook);
    const int boundAfterViews = hook.calls().back().drawFramebuffer;

    TechEngine::ViewRequest zeroScene = scene;
    zeroScene.width = 0;
    const std::vector<TechEngine::ViewRequest> zeroSized{zeroScene, game};
    renderer.setViews(zeroSized);
    const std::vector<TechEngine::ImGuiViewTexture> withZero = texturesAfterTwoMoreDraws(hook);
    renderer.stop();

    TechEngineTests::RecordingImGuiHook nextHook;
    REQUIRE(renderer.start(fixture.jobs, fixture.clock, fixture.window, fixture.input, {}, &nextHook));
    REQUIRE(nextHook.waitForDraws(3));
    const std::vector<TechEngine::ImGuiViewTexture> nextSession = nextHook.calls().back().viewTextures;
    renderer.stop();

    CHECK(beforeViews.empty());
    REQUIRE(withBoth.size() == 2);
    for (const TechEngine::ImGuiViewTexture& texture: withBoth) {
        CHECK((texture.token == SCENE_TOKEN || texture.token == GAME_TOKEN));
        CHECK(texture.nativeTexture != 0);
    }
    const bool distinct = withBoth[0].token != withBoth[1].token && withBoth[0].nativeTexture != withBoth[1].nativeTexture;
    CHECK(distinct);
    CHECK(boundAfterViews == 0);
    REQUIRE(withZero.size() == 1);
    CHECK(withZero.front().token == GAME_TOKEN);
    CHECK(nextSession.empty());
}
