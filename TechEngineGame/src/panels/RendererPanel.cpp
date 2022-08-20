#include <iostream>
#include "RendererPanel.hpp"
#include "../wrapper/Wrapper.hpp"
#include "../events/window/WindowCloseEvent.hpp"

namespace TechEngine {
    RendererPanel::RendererPanel() : window("Main", 1200, 600), Panel("Game", true) {
        this->mainPanel = true;

        glfwSetWindowUserPointer(window.getHandler(), this);

        glfwSetWindowCloseCallback(window.getHandler(), [](GLFWwindow *handler) {
            TechEngine::dispatchEvent(new WindowCloseEvent((Panel *) (glfwGetWindowUserPointer(handler))));
        });
        glfwSetKeyCallback(window.getHandler(), [](GLFWwindow *handler, int key, int scancode, int action, int mods) {
            Window::windowKeyInput(key, action);
        });
        glfwSetFramebufferSizeCallback(window.getHandler(), [](GLFWwindow *handler, int width, int height) {
            TechEngine::dispatchEvent(new WindowResizeEvent(width, height));
        });
    }


    void RendererPanel::onUpdate() {
        window.getRenderer().renderPipeline();
        window.onUpdate();
    }

    Window &RendererPanel::getWindow() {
        return window;
    }
}
