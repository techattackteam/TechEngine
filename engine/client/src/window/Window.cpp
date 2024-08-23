#include "Window.hpp"

#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/application/AppCloseEvent.hpp"

namespace TechEngine {
    Window::Window(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void Window::init(const std::string& title, uint32_t width, uint32_t height) {
        m_title = title;
        m_width = width;
        m_height = height;

        glfwInit();
        m_handler = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
        glfwMakeContextCurrent(m_handler);
        if (glewInit() != GLEW_OK) {
            TE_LOGGER_CRITICAL("Failed to initialize GLEW");
        }


        glfwSetWindowUserPointer(m_handler, this);
        glfwSetWindowCloseCallback(m_handler, [](GLFWwindow* handler) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->m_systemsRegistry.getSystem<EventDispatcher>().dispatch(new AppCloseEvent());
        });
    }

    void Window::onUpdate() {
        glfwPollEvents();
        glfwSwapBuffers(m_handler);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Window::onFixedUpdate() {
        System::onFixedUpdate();
    }

    void Window::shutdown() {
        glfwTerminate();
    }

    void Window::setTitle(const std::string& title) {
        this->m_title = title;
        glfwSetWindowTitle(m_handler, title.c_str());
    }

    void Window::setSize(uint32_t width, uint32_t height) {
        this->m_width = width;
        this->m_height = height;
    }
}
