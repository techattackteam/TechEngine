#include "PanelCreateEvent.hpp"

namespace Engine {
    PanelCreateEvent::PanelCreateEvent(Panel *panel) : Event(eventType) {
        this->panel = panel;
    }

    PanelCreateEvent::~PanelCreateEvent() = default;

    Panel *Engine::PanelCreateEvent::getPanel() {
        return panel;
    }

}
