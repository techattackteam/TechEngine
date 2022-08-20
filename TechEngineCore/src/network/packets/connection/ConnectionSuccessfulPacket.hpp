#pragma once

#include <boost/serialization/export.hpp>
#include "../../Packet.hpp"

namespace TechEngineCore {

    class ConnectionSuccessfulPacket : public Packet {
    private:

    public:
        ConnectionSuccessfulPacket() = default;

        ConnectionSuccessfulPacket(const std::string &uuid);

        ~ConnectionSuccessfulPacket() = default;

        void onPacketReceive() override;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version);
    };
}
BOOST_CLASS_EXPORT_KEY(TechEngineCore::ConnectionSuccessfulPacket);
