#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace TechEngineCore {
    using boost::asio::ip::udp;

    class Packet {
    public:
        std::string uuid;
        std::string data;

        int bytes = 0;
        udp::endpoint senderEndpoint;

        Packet() = default;

        virtual ~Packet() = default;

        virtual void onPacketReceive() = 0;

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive &, unsigned int version);
    };
}