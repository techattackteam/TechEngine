#include "Client.hpp"

namespace TechEngineServer {
    Client::Client(std::string UUID, udp::endpoint endPoint) {
        this->UUID = std::move(UUID);
        this->endpoint = std::move(endPoint);
        lastPingTime = std::chrono::system_clock::now();
    }

    std::chrono::system_clock::time_point Client::getLastPingTime() {
        return lastPingTime;
    }

    void Client::setLastPingTime(std::chrono::system_clock::time_point timeStamp) {
        this->lastPingTime = timeStamp;
    }
}
