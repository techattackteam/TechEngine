#pragma once

#include "event/events/Event.hpp"

namespace TechEngine {
    class PingEvent : public Event {
    private:
        std::string uuid;
    public:
        inline static EventType eventType = EventType("PingEvent", ASYNC);

        explicit PingEvent(const std::string &uuid) : uuid(uuid), Event(eventType) {
        }

        ~PingEvent() = default;

        const std::string &getUUID() {
            return uuid;
        }
    };
}
