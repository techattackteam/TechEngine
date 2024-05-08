#include <iostream>
#include "Window.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "Mouse.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "events/window/WindowResizeEvent.hpp"

namespace TechEngine {
    Window::Window(EventDispatcher& eventDispatcher, const std::string& title, uint32_t width, uint32_t height) : eventDispatcher(eventDispatcher), mouse(eventDispatcher) {
        init(title, width, height);
    }

    Window::~Window() {
        glfwMakeContextCurrent(handler);
    }

    void Window::init(const std::string& title, uint32_t width, uint32_t height) {
        this->title = title;
        glfwInit();

        handler = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(handler);

        if (glewInit() != GLEW_OK) {
            std::cout << "Error!" << std::endl;
        }
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glfwSwapInterval(1);
        glfwSetWindowUserPointer(handler, this);
        glfwSetWindowCloseCallback(handler, [](GLFWwindow* handler) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->eventDispatcher.dispatch(new WindowCloseEvent());
            window->eventDispatcher.dispatch(new AppCloseRequestEvent());
        });
        glfwSetKeyCallback(handler, [](GLFWwindow* handler, int key, int scancode, int action, int mods) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->windowKeyInput(key, action);
        });
        glfwSetMouseButtonCallback(handler, [](GLFWwindow* handler, int key, int action, int mods) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->windowKeyInput(key, action);
        });
        glfwSetScrollCallback(handler, [](GLFWwindow* handler, double xoffset, double yoffset) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->eventDispatcher.dispatch(new MouseScrollEvent(xoffset, yoffset));
        });
        glfwSetCursorPosCallback(handler, [](GLFWwindow* handler, double xpos, double ypos) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->mouse.onMouseMove(xpos, ypos);
        });
        glfwSetFramebufferSizeCallback(handler, [](GLFWwindow* handler, int width, int height) {
            Window* window = (Window*)glfwGetWindowUserPointer(handler);
            window->eventDispatcher.dispatch(new WindowResizeEvent(width, height));
        });
    }

    void Window::onUpdate() {
        mouse.onUpdate();
        glfwSwapBuffers(handler);
        glfwPollEvents();
    }

    void Window::setVSync(bool enabled) {
        vSync = enabled;
    }

    bool Window::isVSync() {
        return vSync;
    }

    GLFWwindow* Window::getHandler() {
        return handler;
    }

    void Window::windowKeyInput(int key, int action) {
        switch (action) {
            case GLFW_PRESS: {
                eventDispatcher.dispatch(new KeyPressedEvent(Key(key)));
                break;
            }
            case GLFW_RELEASE: {
                eventDispatcher.dispatch(new KeyReleasedEvent(Key(key)));
                break;
            }
            case GLFW_REPEAT: {
                eventDispatcher.dispatch(new KeyHoldEvent(Key(key)));
                break;
            }
        }
    }

    void Window::changeTitle(const std::string& name) {
        this->title = name;
    }
}
