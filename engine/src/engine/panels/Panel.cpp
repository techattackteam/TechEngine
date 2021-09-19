#include <iostream>
#include "Panel.hpp"

namespace Engine {
    Panel::Panel(const std::string &name, bool mainPanel) {
        this->mainPanel = mainPanel;
        this->name = name;
    }

    Panel::~Panel() {
    }


    bool Panel::isMainPanel() const {
        return mainPanel;
    }
}
