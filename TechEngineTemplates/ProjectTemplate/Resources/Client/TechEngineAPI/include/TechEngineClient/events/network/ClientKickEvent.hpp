#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class ClientKickEvent : public Event {
    public:
        inline static EventType eventType = EventType("ClientKickEvent", EventTiming::ASYNC);

        ClientKickEvent() : Event(eventType) {
        };
    };
}
