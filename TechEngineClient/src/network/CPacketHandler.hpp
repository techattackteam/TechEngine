#pragma once

#include "network/handler/PacketHandler.hpp"

namespace TechEngine {
    class CPacketHandler : public TechEngine::PacketHandler {
    public:
        CPacketHandler(TechEngine::NetworkHandler *networkHandler);
    };
}
