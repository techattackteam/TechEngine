#include <iostream>
#include "Panel.hpp"
#include "../event/events/panels/PanelCreateEvent.hpp"

namespace Engine {
    Panel::Panel(const std::string &name, bool mainPanel) {
        EventDispatcher::getInstance().dispatch(new PanelCreateEvent(this));
        this->mainPanel = mainPanel;
        this->name = name;
    }

    Panel::~Panel() {
    }


    bool Panel::isMainPanel() const {
        return mainPanel;
    }
}
