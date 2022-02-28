#pragma once

#include "../Event.hpp"
#include "../../../panels/Panel.hpp"

namespace Engine {
    class WindowCloseEvent : public Event {
    private:
        Panel *panel;
    public:

        inline static EventType eventType = EventType("WindowCloseEvent", SYNC);

        explicit WindowCloseEvent(Panel *window) : Event(eventType) {
            panel = window;
        };

        ~WindowCloseEvent() override = default;

        Panel *getPanel() {
            return panel;
        }

    };
}


