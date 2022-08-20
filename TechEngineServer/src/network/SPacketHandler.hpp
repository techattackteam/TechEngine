#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"

namespace TechEngineServer {
    class SPacketHandler : public PacketHandler {
    private:

    public:

        SPacketHandler(SNetworkHandler *networkHandler);

        void init() override;

    };
}
