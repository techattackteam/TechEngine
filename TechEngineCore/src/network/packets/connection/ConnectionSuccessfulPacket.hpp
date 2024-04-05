#pragma once

#include "network/Packet.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/connection/ConnectionSuccessfulEvent.hpp"

namespace TechEngine {

    class ConnectionSuccessfulPacket : public Packet {
    public:
        ConnectionSuccessfulPacket() = default;

        ~ConnectionSuccessfulPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}
