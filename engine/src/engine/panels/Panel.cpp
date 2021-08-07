#include <iostream>
#include "Panel.hpp"
#include "../event/events/panels/PanelCreateEvent.hpp"
#include "../../wrapper/Wrapper.hpp"
#include "../event/events/window/WindowCloseEvent.hpp"

namespace Engine {
    Panel::Panel(const std::string &name, bool mainPanel) : window(name, 800, 600) {
        EventDispatcher::getInstance().dispatch(new PanelCreateEvent(this));
        this->mainPanel = mainPanel;

        glfwSetWindowUserPointer(window.getHandler(), this);
        glfwSetWindowCloseCallback(window.getHandler(), [](GLFWwindow *handler) {
            Engine::dispatchEvent(new WindowCloseEvent((Panel *) (glfwGetWindowUserPointer(handler))));
        });
    }

    Panel::~Panel() {
    }


    bool Panel::isMainPanel() const {
        return mainPanel;
    }
}
