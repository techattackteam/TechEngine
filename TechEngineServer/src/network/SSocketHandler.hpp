#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"

namespace TechEngineServer {

    class SSocketHandler : public SocketHandler {
    private:
        SNetworkHandler *networkHandler;
    public:
        explicit SSocketHandler(SNetworkHandler *networkHandler);

        void init() override;
    };
}
