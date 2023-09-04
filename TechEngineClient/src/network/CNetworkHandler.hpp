#pragma once

#include "network/handler/NetworkHandler.hpp"

namespace TechEngine {
    class CNetworkHandler : public TechEngine::NetworkHandler {
    private:
        udp::endpoint serverEndpoint;

        std::string uuid;
        std::string serverIP;
        short serverPort;
        short port;
    public:
        CNetworkHandler(std::string serverIp, const short &serverPort, const short &port);

        void init() override;

        void sendPacket(TechEngine::Packet *packet);

        udp::endpoint getServerEndpoint() const;

        void connectWithServer();

        void setUUID(std::string uuid);

        const std::string &getUUID();

        const short &getPort();

        void setPort(short port);

    };
}