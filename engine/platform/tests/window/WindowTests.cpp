#include <TechEngine/platform/window/Window.hpp>

#include <catch2/catch_test_macros.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <thread>

struct PlatformWindowTestScope {
    ~PlatformWindowTestScope() {
        TechEngine::Window::terminate();
    }
};

TEST_CASE("Window opens and closes on main without claiming the context", "[platform][window]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Window test"));
    CHECK(glfwGetCurrentContext() == nullptr);
    CHECK_FALSE(window.shouldClose());
    CHECK_FALSE(window.open(320, 240, "Duplicate open"));
    window.pollEvents();
    CHECK(glfwGetCurrentContext() == nullptr);
    window.close();
    CHECK(window.shouldClose());
    window.close();
    REQUIRE(window.open(320, 240, "Reopened window"));
    CHECK(glfwGetCurrentContext() == nullptr);
}

TEST_CASE("Window releases a context on its owning worker", "[platform][window]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Context test"));
    bool claimed = false;
    bool released = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            claimed = glfwGetCurrentContext() != nullptr;
            window.releaseContext();
            released = glfwGetCurrentContext() == nullptr;
        });
    }
    CHECK(claimed);
    CHECK(released);
    CHECK(glfwGetCurrentContext() == nullptr);
}
