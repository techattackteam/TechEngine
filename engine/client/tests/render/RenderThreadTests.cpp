#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <algorithm>
#include <thread>
#include <vector>

struct RenderWindowTestScope {
    ~RenderWindowTestScope() {
        TechEngine::Window::terminate();
    }
};

TEST_CASE("Render thread loads GL with its context current", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render startup test"));
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(jobs, window));
    CHECK(GLAD_GL_VERSION_4_5 != 0);
    CHECK_FALSE(renderer.start(jobs, window));
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
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render shutdown test"));
    {
        TechEngine::RenderThread renderer;
        REQUIRE(renderer.start(jobs, window));
        renderer.stop();
        renderer.stop();
        REQUIRE(renderer.start(jobs, window));
    }
    window.close();
    CHECK(window.shouldClose());
}

TEST_CASE("Render thread joins after failed startup and can retry", "[client][render][window]") {
    TechEngine::JobSystem jobs{1};
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    TechEngine::RenderThread renderer;
    CHECK_FALSE(renderer.start(jobs, window));
    REQUIRE(jobs.registeredThreads().size() == 1);
    CHECK(jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    REQUIRE(window.open(320, 240, "Startup retry test"));
    REQUIRE(renderer.start(jobs, window));
    renderer.stop();
    REQUIRE(jobs.registeredThreads().size() == 1);
    CHECK(jobs.registeredThreads().front().role == TechEngine::ThreadRole::PoolWorker);
    window.close();
}
