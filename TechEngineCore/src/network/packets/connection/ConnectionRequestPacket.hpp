#pragma once

#include "network/Packet.hpp"

namespace TechEngine {
    class ConnectionRequestPacket : public Packet {
    private:
    public:
        ConnectionRequestPacket() = default;

        ~ConnectionRequestPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}
