#pragma once

#include "network/Packet.hpp"
#include "event/events/connection/ConnectionRequestEvent.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngineCore {
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
