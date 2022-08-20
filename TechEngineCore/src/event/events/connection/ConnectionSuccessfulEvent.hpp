#pragma once

#include <utility>

#include "../Event.hpp"
#include "event/events/Event.hpp"

namespace TechEngineCore {
    class ConnectionSuccessfulEvent : public Event {
    private:
        const std::string uuid;
    public:
        inline static EventType eventType = EventType("ConnectionSuccessfulEvent", ASYNC);

        explicit ConnectionSuccessfulEvent(std::string uuid) : uuid(std::move(uuid)), Event(eventType) {
        }

        ~ConnectionSuccessfulEvent() = default;


    };
}