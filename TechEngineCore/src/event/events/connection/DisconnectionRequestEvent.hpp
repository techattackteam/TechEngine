#pragma once

#include "event/events/Event.hpp"
#include "core/Core.hpp"
namespace TechEngineCore {
    class /*Engine_API*/ DisconnectionRequestEvent : public Event {
    private:
        std::string uuid;
        udp::endpoint endpoint;
    public:
        inline static EventType eventType = EventType("DisconnectionRequestEvent", ASYNC);

        explicit DisconnectionRequestEvent(const std::string &uuid, const udp::endpoint &endpoint) : Event(eventType) {

        }

        ~DisconnectionRequestEvent() override = default;

        inline std::string getUUID() {
            return uuid;
        }

        udp::endpoint getEndpoint() {
            return endpoint;
        }
    };
}
