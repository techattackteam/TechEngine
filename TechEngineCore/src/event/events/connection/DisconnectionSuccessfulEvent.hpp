#pragma once

#include "event/events/Event.hpp"
#include "core/Core.hpp"
namespace TechEngineCore {
    class Engine_API DisconnectionSuccessfulEvent : public Event {
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
