#pragma once

#include <events/Event.hpp>

namespace TechEngine {
    class OnClientDisconnected : public Event {
    public:
        inline static EventType eventType = EventType("OnClientDisconnected", EventTiming::SYNC);
        const ClientID& clientID;

        OnClientDisconnected(const ClientID& clientID) : clientID(clientID), Event(eventType) {
        };

        ~OnClientDisconnected() override = default;

        const ClientID& getClientID() const {
            return clientID;
        }
    };
}
