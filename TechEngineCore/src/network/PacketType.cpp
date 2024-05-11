#include "PacketType.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    bool checkCustomPacketType(const std::string& type) {
        if (type.empty() || std::find(customPacketTypes.begin(), customPacketTypes.end(), type) != customPacketTypes.end()) {
            TE_LOGGER_WARN("Invalid custom packet type: {}", type);
            return false;
        } else {
            return true;
        }
    }

    std::string_view PacketTypeToString(PacketType type) {
        switch (type) {
            case PacketType::None: return "PacketType::None";
            case PacketType::Message: return "PacketType::Message";
            case PacketType::ClientConnectionRequest: return "PacketType::ClientConnectionRequest";
            case PacketType::ConnectionStatus: return "PacketType::ConnectionStatus";
            case PacketType::ClientList: return "PacketType::ClientList";
            case PacketType::ClientConnect: return "PacketType::ClientConnect";
            case PacketType::ClientUpdate: return "PacketType::ClientUpdate";
            case PacketType::ClientDisconnect: return "PacketType::ClientDisconnect";
            case PacketType::ClientUpdateResponse: return "PacketType::ClientUpdateResponse";
            case PacketType::MessageHistory: return "PacketType::MessageHistory";
            case PacketType::ServerShutdown: return "PacketType::ServerShutdown";
            case PacketType::ClientKick: return "PacketType::ClientKick";
            case PacketType::ClientBan: return "PacketType::ClientBan";
            case PacketType::SyncGameObject: return "PacketType::SyncGameObject";
            default: return "PacketType::<Invalid>";
        }

        return "PacketType::<Invalid>";
    }
}
