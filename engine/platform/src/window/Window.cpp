#include <TechEngine/platform/window/Window.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>

namespace TechEngine {
    Window::Window() = default;

    Window::~Window() {
        close();
    }

    bool Window::initialize() {
        if (!glfwInit()) {
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        return true;
    }

    bool Window::open(int width, int height, std::string_view const title) {
        if (m_window != nullptr) {
            return false;
        }
        const std::string windowTitle{title};
        m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
        return m_window != nullptr;
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
    }

    bool Window::shouldClose() const {
        return m_window == nullptr || glfwWindowShouldClose(m_window) != 0;
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

    GlProcLoader Window::processLoader() const {
        return glfwGetProcAddress;
    }
}
