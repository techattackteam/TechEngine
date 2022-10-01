#pragma once

#include <TechEngineCore.hpp>
//#include "panels/Panel.hpp"

namespace TechEngine {
    class WindowCloseEvent : public TechEngineCore::Event {
    private:
        //Panel *panel;
    public:

        inline static EventType eventType = EventType("WindowCloseEvent", SYNC);

        explicit WindowCloseEvent() : Event(eventType) {
        };

        ~WindowCloseEvent() override = default;


    };
}


