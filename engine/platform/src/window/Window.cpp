#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/window/Window.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
        return true;
    }

    void Window::terminate() {
        glfwTerminate();
    }

    void Window::pollEvents() {
        glfwPollEvents();
    }

    void Window::setTitle(std::string_view title) {
        if (m_window != nullptr) {
            const std::string windowTitle{title};
            glfwSetWindowTitle(m_window, windowTitle.c_str());
        }
    }

    void Window::close() {
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
}
