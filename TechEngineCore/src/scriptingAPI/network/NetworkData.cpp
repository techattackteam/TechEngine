#include "NetworkData.hpp"

#include "network/PacketType.hpp"

namespace TechEngine::NetworkData {
    void registerPacketType(const std::string& packetName) {
        customPacketTypes.push_back(packetName);
    }
}
