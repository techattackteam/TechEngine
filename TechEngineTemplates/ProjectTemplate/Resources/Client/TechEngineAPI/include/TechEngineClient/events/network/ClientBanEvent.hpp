#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class ClientBanEvent : public Event {
    public:
        inline static EventType eventType = EventType("ClientBanEvent", EventTiming::ASYNC);

        ClientBanEvent() : Event(eventType) {
        };
    };
}