#include <iostream>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <imgui.h>
#include "Window.hpp"
#include "../events/input/KeyPressedEvent.hpp"
#include "../events/input/KeyReleasedEvent.hpp"
#include "../events/input/KeyHoldEvent.hpp"

namespace TechEngine {
    Window::Window(const std::string &title, int width, int height) : settings() {
        WindowSettings::title = title;
        WindowSettings::width = width;
        WindowSettings::height = height;
        WindowSettings::aspectRatio = (float) width / (float) height;
        glfwInit();

        handler = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(handler);

        if (glewInit() != GLEW_OK) {
            std::cout << "Error!" << std::endl;
        }
        //glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
        //glDepthMask(GL_TRUE);
        //glCullFace(GL_BACK);

        TechEngineCore::EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngineCore::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) event);
        });

        glViewport(0, 0, width, height);
    }

    Window::~Window() {
        glfwMakeContextCurrent(handler);
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
        glViewport(0, 0, event->getWidth(), event->getHeight());
        WindowSettings::width = event->getWidth();
        WindowSettings::height = event->getHeight();
        WindowSettings::aspectRatio = (float) event->getWidth() / (float) event->getHeight();
    }
}