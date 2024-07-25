#include "NetworkData.hpp"

#include "network/PacketType.hpp"

namespace TechEngine {
    void NetworkData::registerPacketType(const std::string& packetName) {
        customPacketTypes.push_back(packetName);
    }

    const int& NetworkData::getNetworkID() {
        return networkID;
    }
}
