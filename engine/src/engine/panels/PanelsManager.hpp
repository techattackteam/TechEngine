#pragma once

#include "Panel.hpp"
#include "../event/events/panels/PanelCreateEvent.hpp"

namespace Engine {
    class PanelsManager {
    private:
        std::vector<Panel *> panels;
    public:
        PanelsManager();

        void update();

        void registerPanel(PanelCreateEvent *);

        void unregisterPanel(Panel *panel);
    };
}


