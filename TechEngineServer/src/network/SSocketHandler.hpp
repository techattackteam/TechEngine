#pragma once

#include "SNetworkHandler.hpp"

namespace TechEngineServer {

    class SSocketHandler : public TechEngineCore::SocketHandler {
    private:
        SNetworkHandler *networkHandler;
    public:
        explicit SSocketHandler(SNetworkHandler *networkHandler);

        void init() override;
    };
}
