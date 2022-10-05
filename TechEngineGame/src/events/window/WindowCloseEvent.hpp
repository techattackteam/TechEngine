#pragma once

#include <TechEngineCore.hpp>

namespace TechEngine {
    class WindowCloseEvent : public TechEngineCore::Event {
    public:
        inline static EventType eventType = EventType("WindowCloseEvent", SYNC);

        explicit WindowCloseEvent() : Event(eventType) {
        };

        ~WindowCloseEvent() override = default;


    };
}


