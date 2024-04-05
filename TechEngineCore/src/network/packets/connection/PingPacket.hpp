#pragma once

#include "network/Packet.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/connection/PingEvent.hpp"

namespace TechEngine {

    class PingPacket : public Packet {
    public:
        PingPacket() = default;

        ~PingPacket() override = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}

