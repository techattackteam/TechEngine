#pragma once

#include <iostream>

namespace TechEngine {
    class RegisterCustomPanel : public TechEngineCore::Event {
    private:
        CustomPanel *panel;
    public:
        inline static EventType eventType = EventType("RegisterCustomPanel", SYNC);

        explicit RegisterCustomPanel(CustomPanel *panel) : Event(eventType) {
            this->panel = panel;
        };

        ~RegisterCustomPanel() override = default;

        CustomPanel *getPanel() {
            return panel;
        };
    };

}


