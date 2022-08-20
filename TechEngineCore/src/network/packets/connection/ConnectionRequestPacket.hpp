#pragma once

#include <boost/serialization/export.hpp>
#include "../../Packet.hpp"

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
BOOST_CLASS_EXPORT_KEY(TechEngineCore::ConnectionRequestPacket);
