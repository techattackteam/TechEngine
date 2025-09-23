#include "Window.hpp"

#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/application/AppCloseEvent.hpp"
#include "renderer/ErrorCatcher.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    Window::Window(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void Window::init(const std::string& title, uint32_t width, uint32_t height) {
        m_title = title;
        m_width = width;
        m_height = height;

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef TECH_ENGINE_CORE_DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // Request a debug context!
#endif

        m_handler = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
        glfwMakeContextCurrent(m_handler);
        if (glewInit() != GLEW_OK) {
            TE_LOGGER_CRITICAL("Failed to initialize GLEW");
        }

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Makes callbacks synchronous for easier debugging
            glDebugMessageCallback(DebugMessageCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            TE_LOGGER_INFO("OpenGL Debug Context Initialized");
        }

        glfwSetWindowUserPointer(m_handler, this);
        glfwSetWindowCloseCallback(m_handler, [](GLFWwindow* handler) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->m_systemsRegistry.getSystem<EventDispatcher>().dispatch<AppCloseEvent>();
        });

        glfwSetFramebufferSizeCallback(m_handler, [](GLFWwindow* handler, int width, int height) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            //window->m_systemsRegistry.getSystem<EventDispatcher>().dispatch<WindowRes>(new WindowResizeEvent(width, height));
        });
        glfwSwapInterval(0);
    }

    void Window::onUpdate() {
        glfwPollEvents();
        glfwSwapBuffers(m_handler);
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
