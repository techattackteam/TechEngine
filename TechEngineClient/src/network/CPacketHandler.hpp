#pragma once

#include "network/handler/PacketHandler.hpp"

namespace TechEngine {
    class CPacketHandler : public TechEngineCore::PacketHandler {
    public:
        CPacketHandler(TechEngineCore::NetworkHandler *networkHandler);
    };
}
