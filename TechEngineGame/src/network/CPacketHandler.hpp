#pragma once

#include "tegpph.hpp"

namespace TechEngine {
    class CPacketHandler : public PacketHandler {
    public:
        CPacketHandler(NetworkHandler *networkHandler);
    };
}

