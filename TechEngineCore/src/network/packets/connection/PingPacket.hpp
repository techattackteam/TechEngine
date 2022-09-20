#pragma once

#include <boost/serialization/export.hpp>
#include "network/Packet.hpp"

namespace TechEngineCore {

    class PingPacket : public Packet {
    public:
        PingPacket() = default;

        PingPacket(const std::string &uuid);

        ~PingPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}

BOOST_CLASS_EXPORT_KEY(TechEngineCore::PingPacket);
