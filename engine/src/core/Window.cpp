#include <iostream>
#include "Window.hpp"
#include "../event/EventDispatcher.hpp"
#include "../event/events/input/KeyPressedEvent.hpp"
#include "../event/events/input/KeyReleasedEvent.hpp"
#include "../event/events/input/KeyHoldEvent.hpp"

namespace Engine {
    Window::Window(std::string title, int width, int height) {
        this->title = title;
        this->width = width;
        this->height = height;

        glfwInit();

        handler = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(handler);

        if (glewInit() != GLEW_OK) {
            std::cout << "Error!" << std::endl;
        }
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glCullFace(GL_BACK);

        renderer.init(width, height);
    }


    Window::~Window() {
        glfwMakeContextCurrent(handler);
        glfwDestroyWindow(handler);
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

    std::string &Window::getTitle() {
        return title;
    }

    void Window::windowKeyInput(int key, int action) {
        switch (action) {
            case GLFW_PRESS: {
                EventDispatcher::getInstance().dispatch(new KeyPressedEvent(Key(Key::getKeyCode(key))));
                break;
            }
            case GLFW_RELEASE: {
                EventDispatcher::getInstance().dispatch(new KeyReleasedEvent(Key(Key::getKeyCode(key))));
                break;
            }
            case GLFW_REPEAT: {
                EventDispatcher::getInstance().dispatch(new KeyHoldEvent(Key(Key::getKeyCode(key))));
                break;
            }
        }
    }

    Renderer &Window::getRenderer() {
        return renderer;
    }


}