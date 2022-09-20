#pragma once

#include <boost/serialization/export.hpp>
#include "network/Packet.hpp"

namespace TechEngineCore {

    class DisconnectionSuccessfulPacket : public Packet {
    private:

    public:
        DisconnectionSuccessfulPacket() = default;

        DisconnectionSuccessfulPacket(const std::string &uuid);

        ~DisconnectionSuccessfulPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}
BOOST_CLASS_EXPORT_KEY(TechEngineCore::DisconnectionSuccessfulPacket);
