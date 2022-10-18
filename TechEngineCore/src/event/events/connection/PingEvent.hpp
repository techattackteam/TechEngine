#pragma once

#include "event/events/Event.hpp"
#include "core/Core.hpp"

namespace TechEngineCore {
    class Engine_API PingEvent : public Event {
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
