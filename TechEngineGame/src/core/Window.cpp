#include <iostream>
#include "Window.hpp"
#include "../events/input/KeyPressedEvent.hpp"
#include "../events/input/KeyReleasedEvent.hpp"
#include "../events/input/KeyHoldEvent.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "wrapper/Wrapper.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"

namespace TechEngine {
    Window::Window(const std::string &title, uint32_t width, uint32_t height) {
        init(title, width, height);
    }

    Window::~Window() {
        glfwMakeContextCurrent(handler);
    }

    void Window::init(const std::string &title, uint32_t width, uint32_t height) {
        this->title = title;
        this->width = width;
        this->height = height;
        this->aspectRatio = (float) width / (float) height;
        glfwInit();

        handler = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(handler);

        if (glewInit() != GLEW_OK) {
            std::cout << "Error!" << std::endl;
        }
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_BACK);

        glfwSetWindowUserPointer(handler, this);
        glfwSetWindowCloseCallback(handler, [](GLFWwindow *handler) {
            TechEngine::dispatchEvent(new WindowCloseEvent());
            TechEngine::dispatchEvent(new AppCloseRequestEvent());
        });
        glfwSetKeyCallback(handler, [](GLFWwindow *handler, int key, int scancode, int action, int mods) {
            Window::windowKeyInput(key, action);
        });
        glfwSetFramebufferSizeCallback(handler, [](GLFWwindow *handler, int width, int height) {
            TechEngine::dispatchEvent(new WindowResizeEvent(width, height));
        });
        TechEngineCore::EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngineCore::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) event);
        });
    }

    void Window::onUpdate() {
        glfwSwapBuffers(handler);
        glfwPollEvents();
    }

    void Window::setVSync(bool enabled) {
        vSync = enabled;
    }

    bool Window::isVSync() {
        return vSync;
    }

    GLFWwindow *Window::getHandler() {
        return handler;
    }

    void Window::windowKeyInput(int key, int action) {
        switch (action) {
            case GLFW_PRESS: {
                TechEngineCore::EventDispatcher::getInstance().dispatch(new KeyPressedEvent(Key(Key::getKeyCode(key))));
                break;
            }
            case GLFW_RELEASE: {
                TechEngineCore::EventDispatcher::getInstance().dispatch(new KeyReleasedEvent(Key(Key::getKeyCode(key))));
                break;
            }
            case GLFW_REPEAT: {
                TechEngineCore::EventDispatcher::getInstance().dispatch(new KeyHoldEvent(Key(Key::getKeyCode(key))));
                break;
            }
        }
    }

    void Window::onWindowResizeEvent(WindowResizeEvent *event) {
        this->width = event->getWidth();
        this->height = event->getHeight();
        this->aspectRatio = (float) event->getWidth() / (float) event->getHeight();
    }

    void Window::changeTitle(const std::string &name) {
        this->title = name;
    }

    Renderer &Window::getRenderer() {
        return renderer;
    }
}