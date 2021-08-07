#include <iostream>
#include "PanelsManager.hpp"

namespace Engine {
    PanelsManager::PanelsManager() {
        EventDispatcher::getInstance().subscribe(PanelCreateEvent::eventType, [this](Event *event) {
            registerPanel((PanelCreateEvent *) event);
        });

    }

    void PanelsManager::update() {
        for (Panel *panel : panels) {
            panel->onUpdate();
        }
    }

    void PanelsManager::registerPanel(PanelCreateEvent *event) {
        panels.emplace_back(event->getPanel());
    }

    void PanelsManager::unregisterPanel(Panel *panel) {
        auto index = std::find(panels.begin(), panels.end(), panel);
        if (index != panels.end()) {
            panels.erase(index);
        }
        delete (panel);
    }
}
