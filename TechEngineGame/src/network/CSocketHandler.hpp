#pragma once

#include <TechEngineCore.hpp>
#include "CNetworkHandler.hpp"

namespace TechEngine {

    class CSocketHandler : public TechEngineCore::SocketHandler {
    private:
        udp::resolver *resolver;
        CNetworkHandler *networkHandler;

    public:
        CSocketHandler(CNetworkHandler *networkHandler);

        ~CSocketHandler();

        void init() override;

        udp::resolver *getResolver() const;
    };
}
