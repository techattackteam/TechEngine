#pragma once

#include <TechEngineCore.hpp>
#include "../../panels/Panel.hpp"

namespace TechEngine {
    class WindowCloseEvent : public TechEngineCore::Event {
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


