#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <string>

namespace TechEngine {
    static void logGlfwError(int code, const char* description) {
        TE_LOGGER_ERROR("GLFW error {0}: {1}", code, description);
    }

    Window::Window() = default;

    Window::~Window() {
        close();
    }

    bool Window::initialize() {
        glfwSetErrorCallback(logGlfwError);
        if (!glfwInit()) {
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        return true;
    }

    bool Window::open(int width, int height, std::string_view const title) {
        if (m_window != nullptr || width <= 0 || height <= 0) {
            return false;
        }
        const std::string windowTitle{title};
        m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
        if (m_window == nullptr) {
            return false;
        }
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
        framebufferSizeCallback(framebufferWidth, framebufferHeight);
        glfwSetWindowUserPointer(m_window, this);

        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, const int width, const int height) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->framebufferSizeCallback(width, height);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, const int key, int, const int action, int) {
            if (action != GLFW_REPEAT) {
                static_cast<Window*>(glfwGetWindowUserPointer(window))->publishInput(InputEvent{.kind = InputKind::Key, .code = key, .pressed = action == GLFW_PRESS});
            }
        });

        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, const int button, const int action, int) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->publishInput(InputEvent{.kind = InputKind::Button, .code = button, .pressed = action == GLFW_PRESS});
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, const double x, const double y) {
            static_cast<Window*>(glfwGetWindowUserPointer(window))->cursorPositionCallback(x, y);
        });
        glfwSetWindowFocusCallback(m_window, [](GLFWwindow* window, const int focused) {
            Window& owner = *static_cast<Window*>(glfwGetWindowUserPointer(window));
            owner.m_cursorKnown = false;
            owner.publishInput(InputEvent{.kind = InputKind::Focus, .pressed = focused != 0});
        });
        return true;
    }

    void Window::terminate() {
        glfwTerminate();
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    void Window::waitEvents() {
        glfwWaitEvents();
    }

    void Window::waitEvents(const double timeoutSeconds) {
        if (std::isfinite(timeoutSeconds) && timeoutSeconds > 0.0) {
            glfwWaitEventsTimeout(timeoutSeconds);
        } else {
            glfwPollEvents();
        }
    }

    void Window::postEmptyEvent() {
        glfwPostEmptyEvent();
    }

    void Window::setInputBuffer(InputBuffer* input) {
        m_input = input;
        m_cursorKnown = false;
        if (m_window != nullptr) {
            publishInput(InputEvent{.kind = InputKind::Focus, .pressed = glfwGetWindowAttrib(m_window, GLFW_FOCUSED) != 0});
        }
    }

    void Window::setTitle(std::string_view title) {
        if (m_window != nullptr) {
            const std::string windowTitle{title};
            glfwSetWindowTitle(m_window, windowTitle.c_str());
        }
    }

    void Window::close() {
        m_cursorKnown = false;
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        framebufferSizeCallback(0, 0);
    }

    bool Window::shouldClose() const {
        return m_window == nullptr || glfwWindowShouldClose(m_window) != 0;
    }

    FramebufferSize Window::framebufferSize() const {
        const std::lock_guard lock{m_framebufferMutex};
        return m_framebufferSize;
    }

    void Window::makeContextCurrent() const {
        glfwMakeContextCurrent(m_window);
    }

    void Window::releaseContext() {
        glfwMakeContextCurrent(nullptr);
    }

    void Window::swapBuffers() {
        glfwSwapBuffers(m_window);
    }

    void Window::setVSync(bool vsync) {
        glfwSwapInterval(vsync ? 1 : 0);
    }

    GlProcLoader Window::processLoader() const {
        return glfwGetProcAddress;
    }

    void Window::framebufferSizeCallback(int width, int height) {
        const std::lock_guard lock{m_framebufferMutex};
        m_framebufferSize = FramebufferSize{width, height};
    }

    void Window::cursorPositionCallback(const double x, const double y) {
        if (m_cursorKnown) {
            publishInput(InputEvent{.kind = InputKind::Motion, .x = x - m_cursorX, .y = y - m_cursorY});
        }
        m_cursorX = x;
        m_cursorY = y;
        m_cursorKnown = true;
    }

    static const char* inputKindLabel(const InputKind kind) {
        switch (kind) {
            case InputKind::Key:
                return "Key";
            case InputKind::Button:
                return "Button";
            case InputKind::Motion:
                return "Motion";
            case InputKind::Focus:
                return "Focus";
        }
        return "Unknown";
    }

    void Window::publishInput(const InputEvent& event) {
        if (event.kind == InputKind::Motion) {
            TE_LOGGER_TRACE("Input: {0} dx={1:.1f} dy={2:.1f}", inputKindLabel(event.kind), event.x, event.y);
        } else {
            TE_LOGGER_TRACE("Input: {0} code={1} pressed={2}", inputKindLabel(event.kind), event.code, event.pressed);
        }
        if (m_input != nullptr) {
            m_input->publish(event);
        }
    }
}
