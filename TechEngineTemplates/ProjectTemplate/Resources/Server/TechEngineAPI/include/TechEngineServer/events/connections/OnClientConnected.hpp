#pragma once

#include <events/Event.hpp>
#include <core/ClientInfo.hpp>

namespace TechEngine {
    class OnClientConnected : public Event {
    public:
        inline static EventType eventType = EventType("OnClientConnected", EventTiming::SYNC);
        const ClientID& clientID;

        OnClientConnected(const ClientID& clientID) : clientID(clientID), Event(eventType) {
        }

        ~OnClientConnected() override = default;

        const ClientID& getClientID() const {
            return clientID;
        }
    };
}
