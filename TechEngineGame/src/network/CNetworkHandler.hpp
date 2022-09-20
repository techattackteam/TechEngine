#pragma once

#include <TechEngineCore.hpp>
#include "network/packets/connection/PingPacket.hpp"

namespace TechEngine {
    class CNetworkHandler : public TechEngineCore::NetworkHandler {
    private:
        udp::endpoint serverEndpoint;

        std::string uuid;
    public:
        CNetworkHandler(std::string serverIp, const short &port);

        void init() override;

        udp::endpoint getServerEndpoint() const;

        void connectWithServer();

        void setUUID(std::string uuid);

        const std::string &getUUID();
    };
}
