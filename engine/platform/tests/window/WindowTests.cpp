#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <catch2/catch_test_macros.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstddef>
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

struct GlfwInputCallbacks {
    GLFWkeyfun key = nullptr;
    GLFWmousebuttonfun button = nullptr;
    GLFWcursorposfun cursor = nullptr;
    GLFWwindowfocusfun focus = nullptr;
};

static GLFWwindow* nativeHandle(TechEngine::Window& window) {
    GLFWwindow* nativeWindow = nullptr;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            nativeWindow = glfwGetCurrentContext();
            window.releaseContext();
        });
    }
    return nativeWindow;
}

static GlfwInputCallbacks takeInputCallbacks(GLFWwindow* nativeWindow) {
    return GlfwInputCallbacks{
        .key = glfwSetKeyCallback(nativeWindow, nullptr),
        .button = glfwSetMouseButtonCallback(nativeWindow, nullptr),
        .cursor = glfwSetCursorPosCallback(nativeWindow, nullptr),
        .focus = glfwSetWindowFocusCallback(nativeWindow, nullptr),
    };
}

TEST_CASE("Window callbacks publish input into the attached buffer", "[platform][window][input]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    const TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Input test"));

    GLFWwindow* nativeWindow = nativeHandle(window);
    REQUIRE(nativeWindow != nullptr);
    const GlfwInputCallbacks callbacks = takeInputCallbacks(nativeWindow);
    REQUIRE(callbacks.key != nullptr);
    REQUIRE(callbacks.button != nullptr);
    REQUIRE(callbacks.cursor != nullptr);
    REQUIRE(callbacks.focus != nullptr);

    window.setInputBuffer(&input);
    callbacks.focus(nativeWindow, GLFW_TRUE);
    callbacks.cursor(nativeWindow, 10.0, 10.0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
    callbacks.cursor(nativeWindow, 13.0, 6.0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_RELEASE, 0);

    TechEngine::InputFrame frame;
    input.consume(frame);
    REQUIRE(frame.events.size() == 5);
    CHECK(frame.events[0].kind == TechEngine::InputKind::Focus);
    CHECK(frame.events[0].pressed);
    CHECK(frame.events[1].kind == TechEngine::InputKind::Key);
    CHECK(frame.events[1].key == TechEngine::Key::W);
    CHECK(frame.events[1].pressed);
    CHECK(frame.events[2].kind == TechEngine::InputKind::Button);
    CHECK(frame.events[2].button == TechEngine::MouseButton::Left);
    CHECK(frame.events[2].pressed);
    CHECK(frame.events[3].kind == TechEngine::InputKind::Motion);
    CHECK(frame.events[3].x == 3.0);
    CHECK(frame.events[3].y == -4.0);
    CHECK(frame.events[4].kind == TechEngine::InputKind::Key);
    CHECK(frame.events[4].key == TechEngine::Key::W);
    CHECK_FALSE(frame.events[4].pressed);
    CHECK_FALSE(frame.held.isHeld(TechEngine::Key::W));
    CHECK(frame.held.isHeld(TechEngine::MouseButton::Left));
    CHECK(input.presentationState().lookX == 3.0);
}

TEST_CASE("Window translates known controls and drops unknown ones before the buffer", "[platform][window][input]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    const TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Unknown input test"));
    GLFWwindow* nativeWindow = nativeHandle(window);
    REQUIRE(nativeWindow != nullptr);
    const GlfwInputCallbacks callbacks = takeInputCallbacks(nativeWindow);

    window.setInputBuffer(&input);
    callbacks.focus(nativeWindow, GLFW_TRUE);
    callbacks.key(nativeWindow, GLFW_KEY_UNKNOWN, 0, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_LAST + 1, 0, GLFW_PRESS, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_LAST + 1, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_KP_EQUAL, 0, GLFW_PRESS, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_8, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_UNKNOWN, 0, GLFW_RELEASE, 0);

    TechEngine::InputFrame frame;
    input.consume(frame);
    REQUIRE(frame.events.size() == 4);
    CHECK(frame.events[2].kind == TechEngine::InputKind::Key);
    CHECK(frame.events[2].key == TechEngine::Key::KeypadEqual);
    CHECK(frame.events[3].kind == TechEngine::InputKind::Button);
    CHECK(frame.events[3].button == TechEngine::MouseButton::Extra5);
    CHECK(frame.events[2].sequence == frame.events[1].sequence + 1);
    CHECK(frame.events[3].sequence == frame.events[2].sequence + 1);
    CHECK(frame.held.keys.count() == 1);
    CHECK(frame.held.buttons.count() == 1);
    CHECK(frame.held.isHeld(TechEngine::Key::KeypadEqual));
    CHECK(frame.held.isHeld(TechEngine::MouseButton::Extra5));
    CHECK(input.presentationState().sequence == frame.events[3].sequence);
}

TEST_CASE("Window keeps press and release order and never turns GLFW repeat into a press", "[platform][window][input]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    const TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Input order test"));
    GLFWwindow* nativeWindow = nativeHandle(window);
    REQUIRE(nativeWindow != nullptr);
    const GlfwInputCallbacks callbacks = takeInputCallbacks(nativeWindow);

    window.setInputBuffer(&input);
    callbacks.focus(nativeWindow, GLFW_TRUE);
    callbacks.key(nativeWindow, GLFW_KEY_S, 0, GLFW_REPEAT, 0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
    callbacks.key(nativeWindow, GLFW_KEY_A, 0, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_REPEAT, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_RELEASE, 0);
    callbacks.key(nativeWindow, GLFW_KEY_A, 0, GLFW_REPEAT, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, 0);
    callbacks.key(nativeWindow, GLFW_KEY_A, 0, GLFW_RELEASE, 0);

    TechEngine::InputFrame frame;
    input.consume(frame);
    REQUIRE(frame.events.size() == 8);
    CHECK(frame.events[2].key == TechEngine::Key::W);
    CHECK(frame.events[2].pressed);
    CHECK(frame.events[3].key == TechEngine::Key::A);
    CHECK(frame.events[3].pressed);
    CHECK(frame.events[4].kind == TechEngine::InputKind::Button);
    CHECK(frame.events[4].button == TechEngine::MouseButton::Right);
    CHECK(frame.events[4].pressed);
    CHECK(frame.events[5].key == TechEngine::Key::W);
    CHECK_FALSE(frame.events[5].pressed);
    CHECK(frame.events[6].kind == TechEngine::InputKind::Button);
    CHECK_FALSE(frame.events[6].pressed);
    CHECK(frame.events[7].key == TechEngine::Key::A);
    CHECK_FALSE(frame.events[7].pressed);
    for (std::size_t i = 1; i < frame.events.size(); i++) {
        CHECK(frame.events[i - 1].sequence < frame.events[i].sequence);
    }
    CHECK_FALSE(frame.held.isHeld(TechEngine::Key::S));
    CHECK(frame.held.keys.none());
    CHECK(frame.held.buttons.none());
}

TEST_CASE("Window translated controls reach the presentation copy without a simulation consume", "[platform][window][input]") {
    const PlatformWindowTestScope scope;
    REQUIRE(TechEngine::Window::initialize());
    const TechEngine::Clock clock;
    TechEngine::InputBuffer input{clock};
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Presentation input test"));
    GLFWwindow* nativeWindow = nativeHandle(window);
    REQUIRE(nativeWindow != nullptr);
    const GlfwInputCallbacks callbacks = takeInputCallbacks(nativeWindow);

    window.setInputBuffer(&input);
    callbacks.focus(nativeWindow, GLFW_TRUE);
    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_PRESS, 0);
    callbacks.button(nativeWindow, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS, 0);
    CHECK(input.presentationState().isHeld(TechEngine::Key::W));
    CHECK(input.presentationState().isHeld(TechEngine::MouseButton::Middle));

    TechEngine::InputFrame frame;
    input.consume(frame);
    REQUIRE(frame.held.isHeld(TechEngine::Key::W));

    callbacks.key(nativeWindow, GLFW_KEY_W, 0, GLFW_RELEASE, 0);
    CHECK_FALSE(input.presentationState().isHeld(TechEngine::Key::W));
    CHECK(input.presentationState().isHeld(TechEngine::MouseButton::Middle));
    CHECK(frame.held.isHeld(TechEngine::Key::W));

    input.consume(frame);
    CHECK_FALSE(frame.held.isHeld(TechEngine::Key::W));
    CHECK(frame.held.isHeld(TechEngine::MouseButton::Middle));
}
