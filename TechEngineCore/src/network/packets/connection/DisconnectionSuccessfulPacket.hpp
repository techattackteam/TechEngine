#pragma once

#include "network/Packet.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/DisconnectionSuccessfulEvent.hpp"

namespace TechEngineCore {

    class DisconnectionSuccessfulPacket : public Packet {
    public:
        DisconnectionSuccessfulPacket() = default;

        ~DisconnectionSuccessfulPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}
BOOST_CLASS_EXPORT(TechEngineCore::DisconnectionSuccessfulPacket);
