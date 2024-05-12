#pragma once

#include <string>

namespace TechEngine {
    class NetworkData {
    public:
        static void registerPacketType(const std::string& packetName);
    };
}
