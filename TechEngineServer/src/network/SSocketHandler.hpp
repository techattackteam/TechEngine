#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"

namespace TechEngineServer {

    class SSocketHandler : public SocketHandler {
    public:
        explicit SSocketHandler(SNetworkHandler *networkHandler);

        void init() override;
    };
}
