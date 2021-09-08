#include <iostream>
#include "Window.hpp"

namespace Engine {
    Window::Window(std::string title, uint32_t width, uint32_t height) : renderer(width, height) {
        this->title = title;
        this->width = width;
        this->height = height;

        glfwInit();

        handler = glfwCreateWindow((int) width, (int) height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(handler);

        if (glewInit() != GLEW_OK) {
            std::cout << "Error!" << std::endl;
        }
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glCullFace(GL_BACK);
    }


    Window::~Window() {
        glfwMakeContextCurrent(handler);
        glfwDestroyWindow(handler);
    }

    void Window::onUpdate() {
        glfwSwapBuffers(handler);
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

    void Window::takeContext() {
        glfwMakeContextCurrent(handler);
    }

    Renderer &Window::getRenderer() {
        return renderer;
    }
}