#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/asio/ip/udp.hpp>

namespace TechEngineCore {
    using boost::asio::ip::udp;

    using PacketType = std::string;

    class Packet {
    public:
        friend class boost::serialization::access;

        std::string uuid;
        std::string data;

        int bytes = 0;
        udp::endpoint senderEndpoint;

        Packet();

        Packet(const std::string &uuid);

        virtual ~Packet() = default;

        virtual void onPacketReceive() = 0;

        template<class Archive>
        void serialize(Archive &, unsigned int version);
    };
}
