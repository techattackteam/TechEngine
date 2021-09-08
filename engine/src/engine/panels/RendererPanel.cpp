#include "RendererPanel.hpp"
#include "../../wrapper/Wrapper.hpp"
#include "../event/events/window/WindowCloseEvent.hpp"

namespace Engine {
    RendererPanel::RendererPanel() : window("Main", 800, 600), Panel("Game", true) {
        this->mainPanel = true;

        glfwSetWindowUserPointer(window.getHandler(), this);
        glfwSetWindowCloseCallback(window.getHandler(), [](GLFWwindow *handler) {
            Engine::dispatchEvent(new WindowCloseEvent((Panel *) (glfwGetWindowUserPointer(handler))));
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
