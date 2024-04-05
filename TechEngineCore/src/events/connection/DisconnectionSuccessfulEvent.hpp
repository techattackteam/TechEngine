#pragma once

#include "events/Event.hpp"
namespace TechEngine {
    class DisconnectionSuccessfulEvent : public Event {
    private:
        std::string uuid;
    public:
        inline static EventType eventType = EventType("DisconnectionSuccessfulEvent", ASYNC);

        explicit DisconnectionSuccessfulEvent(const std::string &uuid) : Event(eventType) {

        }

        ~DisconnectionSuccessfulEvent() override = default;

        inline std::string getUUID() {
            return uuid;
        }
    };
}
