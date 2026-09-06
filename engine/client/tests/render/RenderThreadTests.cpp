#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <thread>

struct RenderWindowTestScope {
    ~RenderWindowTestScope() {
        TechEngine::Window::terminate();
    }
};

TEST_CASE("Render thread loads GL with its context current", "[client][render][window]") {
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render startup test"));
    TechEngine::RenderThread renderer;
    REQUIRE(renderer.start(window));
    CHECK(GLAD_GL_VERSION_4_5 != 0);
    CHECK_FALSE(renderer.start(window));
    renderer.stop();

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
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Render shutdown test"));
    {
        TechEngine::RenderThread renderer;
        REQUIRE(renderer.start(window));
        renderer.stop();
        renderer.stop();
        REQUIRE(renderer.start(window));
    }
    window.close();
    CHECK(window.shouldClose());
}

TEST_CASE("Render thread joins after failed startup and can retry", "[client][render][window]") {
    const RenderWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    TechEngine::RenderThread renderer;
    CHECK_FALSE(renderer.start(window));
    REQUIRE(window.open(320, 240, "Startup retry test"));
    REQUIRE(renderer.start(window));
    renderer.stop();
    window.close();
}
