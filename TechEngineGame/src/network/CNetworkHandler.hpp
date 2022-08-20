#pragma once

#include <TechEngineCore.hpp>

namespace TechEngine {
    class CNetworkHandler : public TechEngineCore::NetworkHandler {
    private:
        udp::endpoint serverEndpoint;

        int uuid;
    public:
        CNetworkHandler(std::string serverIp, const short &port);

        void init() override;

        udp::endpoint getServerEndpoint() const;

        void connectWithServer();
    };
}
