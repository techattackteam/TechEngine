#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class ConnectionEstablishedEvent : public Event {
    public:
        inline static EventType eventType = EventType("ConnectionEstablishedEvent", EventTiming::ASYNC);

        ConnectionEstablishedEvent() : Event(eventType) {
        };
    };
}