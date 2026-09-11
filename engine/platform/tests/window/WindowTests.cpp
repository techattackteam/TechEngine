#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <catch2/catch_test_macros.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <atomic>
#include <barrier>
#include <chrono>
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

TEST_CASE("Window event waiting returns when another thread posts an empty event", "[platform][window]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Wait test"));
    window.waitEvents(0.0);

    std::atomic<bool> posted = false;
    const auto started = std::chrono::steady_clock::now();
    std::jthread waker([&posted] {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        posted = true;
        TechEngine::Window::postEmptyEvent();
    });
    while (!posted.load()) {
        window.waitEvents(30.0);
    }
    waker.join();
    CHECK(std::chrono::steady_clock::now() - started < std::chrono::seconds{10});
}

TEST_CASE("Window callbacks publish input into the attached buffer", "[platform][window][input]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    const TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Input test"));

    GLFWwindow* nativeWindow = nullptr;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            nativeWindow = glfwGetCurrentContext();
            window.releaseContext();
        });
    }
    REQUIRE(nativeWindow != nullptr);
    const GLFWkeyfun keyCallback = glfwSetKeyCallback(nativeWindow, nullptr);
    const GLFWmousebuttonfun buttonCallback = glfwSetMouseButtonCallback(nativeWindow, nullptr);
    const GLFWcursorposfun cursorCallback = glfwSetCursorPosCallback(nativeWindow, nullptr);
    const GLFWwindowfocusfun focusCallback = glfwSetWindowFocusCallback(nativeWindow, nullptr);
    REQUIRE(keyCallback != nullptr);
    REQUIRE(buttonCallback != nullptr);
    REQUIRE(cursorCallback != nullptr);
    REQUIRE(focusCallback != nullptr);

    window.setInputBuffer(&input);
    focusCallback(nativeWindow, GLFW_TRUE);
    cursorCallback(nativeWindow, 10.0, 10.0);
    keyCallback(nativeWindow, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    keyCallback(nativeWindow, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
    buttonCallback(nativeWindow, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
    cursorCallback(nativeWindow, 13.0, 6.0);
    keyCallback(nativeWindow, GLFW_KEY_W, 0, GLFW_RELEASE, 0);

    TechEngine::InputFrame frame;
    input.consume(frame);
    REQUIRE(frame.events.size() == 6);
    CHECK(frame.events[0].kind == TechEngine::InputKind::Focus);
    CHECK(frame.events[1].kind == TechEngine::InputKind::Focus);
    CHECK(frame.events[1].pressed);
    CHECK(frame.events[2].kind == TechEngine::InputKind::Key);
    CHECK(frame.events[2].pressed);
    CHECK(frame.events[3].kind == TechEngine::InputKind::Button);
    CHECK(frame.events[4].kind == TechEngine::InputKind::Motion);
    CHECK(frame.events[4].x == 3.0);
    CHECK(frame.events[4].y == -4.0);
    CHECK(frame.events[5].kind == TechEngine::InputKind::Key);
    CHECK_FALSE(frame.events[5].pressed);
    CHECK_FALSE(frame.held.keys.test(GLFW_KEY_W));
    CHECK(frame.held.buttons.test(GLFW_MOUSE_BUTTON_LEFT));
    CHECK(input.presentationState().lookX == 3.0);
}
