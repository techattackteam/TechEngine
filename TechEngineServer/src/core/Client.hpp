#pragma once

#include <string>
#include <boost/asio/ip/udp.hpp>

using boost::asio::ip::udp;

namespace TechEngineServer {
    class Client {
    public:
        std::string UUID;

        udp::endpoint endpoint;

        std::chrono::system_clock::time_point lastPingTime;

        Client(std::string UUID, udp::endpoint endPoint);

        std::chrono::system_clock::time_point getLastPingTime();

        void setLastPingTime(std::chrono::system_clock::time_point timeStamp);

    };
}
