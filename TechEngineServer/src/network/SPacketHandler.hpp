#pragma once

#include "SNetworkHandler.hpp"

namespace TechEngineServer {
    class SPacketHandler : public TechEngine::PacketHandler {
    private:

    public:

        SPacketHandler(SNetworkHandler *networkHandler);

        void init() override;

    };
}
