#pragma once

#include "network/Packet.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/PingEvent.hpp"

namespace TechEngineCore {

    class PingPacket : public Packet {
    public:
        PingPacket() = default;

        ~PingPacket() override = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}

