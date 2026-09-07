#include <TechEngine/platform/window/Window.hpp>

#include <catch2/catch_test_macros.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <barrier>
#include <string_view>
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

TEST_CASE("Window title uses the supplied string view and buffers swap on the context owner", "[platform][window]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Original title"));

    GLFWwindow* nativeWindow = nullptr;
    int swapError = GLFW_NO_ERROR;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            nativeWindow = glfwGetCurrentContext();
            if (nativeWindow != nullptr) {
                glfwGetError(nullptr);
                window.swapBuffers();
                swapError = glfwGetError(nullptr);
            }
            window.releaseContext();
        });
    }
    REQUIRE(nativeWindow != nullptr);
    CHECK(swapError == GLFW_NO_ERROR);

    constexpr std::string_view title = "Updated title trailing text";
    window.setTitle(title.substr(0, 13));
    const char* actualTitle = glfwGetWindowTitle(nativeWindow);
    REQUIRE(actualTitle != nullptr);
    CHECK(std::string_view{actualTitle} == "Updated title");
    window.close();
    window.setTitle("Closed window");
    CHECK(window.shouldClose());
}

TEST_CASE("Window publishes framebuffer pixels and callback changes to the render thread", "[platform][window][framebuffer]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    CHECK(window.framebufferSize().width == 0);
    CHECK(window.framebufferSize().height == 0);
    REQUIRE(window.open(320, 240, "Framebuffer size test"));

    GLFWwindow* nativeWindow = nullptr;
    TechEngine::FramebufferSize observed;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            nativeWindow = glfwGetCurrentContext();
            observed = window.framebufferSize();
            window.releaseContext();
        });
    }
    REQUIRE(nativeWindow != nullptr);
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(nativeWindow, &width, &height);
    CHECK(observed.width == width);
    CHECK(observed.height == height);
    CHECK(glfwGetCurrentContext() == nullptr);

    const GLFWframebuffersizefun callback = glfwSetFramebufferSizeCallback(nativeWindow, nullptr);
    REQUIRE(callback != nullptr);
    glfwSetFramebufferSizeCallback(nativeWindow, callback);
    callback(nativeWindow, 640, 360);
    {
        std::jthread worker([&] {
            observed = window.framebufferSize();
        });
    }
    CHECK(observed.width == 640);
    CHECK(observed.height == 360);

    callback(nativeWindow, 1, 2);
    std::barrier start{2};
    bool coherent = true;
    {
        std::jthread worker([&] {
            start.arrive_and_wait();
            for (int i = 0; i < 20000; i++) {
                const TechEngine::FramebufferSize size = window.framebufferSize();
                if (size.width <= 0 || size.height != size.width * 2) {
                    coherent = false;
                }
            }
        });
        start.arrive_and_wait();
        for (int i = 1; i <= 20000; i++) {
            callback(nativeWindow, i, i * 2);
        }
    }
    CHECK(coherent);
    CHECK(window.framebufferSize().width == 20000);
    CHECK(window.framebufferSize().height == 40000);
    callback(nativeWindow, 0, 0);
    CHECK(window.framebufferSize().width == 0);
    CHECK(window.framebufferSize().height == 0);
    callback(nativeWindow, 800, 600);
    CHECK(window.framebufferSize().width == 800);
    CHECK(window.framebufferSize().height == 600);
    CHECK(glfwGetCurrentContext() == nullptr);
    window.close();
    CHECK(window.framebufferSize().width == 0);
    CHECK(window.framebufferSize().height == 0);
    REQUIRE(window.open(160, 120, "Reopened framebuffer test"));
    CHECK(window.framebufferSize().width > 0);
    CHECK(window.framebufferSize().height > 0);
}
