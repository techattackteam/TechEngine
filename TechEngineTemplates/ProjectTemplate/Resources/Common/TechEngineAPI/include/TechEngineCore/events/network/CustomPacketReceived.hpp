#pragma once
#include "events/Event.hpp"

#include <utility>

namespace TechEngine {
    class CustomPacketReceived : public Event {
    public:
        inline static EventType eventType = EventType("CustomPacketReceived", SYNC);

        std::string packet;

        explicit CustomPacketReceived(std::string packet) : packet(std::move(packet)), Event(eventType) {
        }

        const std::string& getPacket() const {
            return packet;
        }
    };
}
