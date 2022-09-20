#pragma once

#include "event/events/Event.hpp"

namespace TechEngineCore {
    class DisconnectionEvent : public Event {
    private:
        std::string uuid;
        udp::endpoint endpoint;
    public:
        inline static EventType eventType = EventType("DisconnectionEvent", ASYNC);

        explicit DisconnectionEvent(const std::string &uuid, const udp::endpoint &endpoint) : Event(eventType) {

        }

        ~DisconnectionEvent() override = default;

        inline std::string getUUID() {
            return uuid;
        }

        udp::endpoint getEndpoint() {
            return endpoint;
        }
    };
}
