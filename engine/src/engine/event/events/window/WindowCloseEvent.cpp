#include "WindowCloseEvent.hpp"


namespace Engine {
    WindowCloseEvent::WindowCloseEvent(Panel *panel) : Event(eventType) {
        this->panel = panel;
    }

    Panel *WindowCloseEvent::getPanel() {
        return panel;
    }
}