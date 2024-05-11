#include "Communicator.hpp"

#include "core/Logger.hpp"
#include "core/Server.hpp"
#include "network/SceneSynchronizer.hpp"

namespace TechEngine {
    Communicator::Communicator(Server& server) : server(server) {
    }

    void Communicator::sendBufferToClient(ClientID clientID, Buffer buffer, bool reliable) {
        EResult result = server.m_interface->SendMessageToConnection((HSteamNetConnection)clientID, buffer.data, (ClientID)buffer.size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
        if (result != k_EResultOK) {
            TE_LOGGER_ERROR("Failed to send message to client {0}", clientID);
        }
    }

    void Communicator::sendBufferToAllClients(Buffer buffer, ClientID excludeClientID, bool reliable) {
        for (const auto& [clientID, clientInfo]: server.m_ConnectedClients) {
            if (clientID != excludeClientID)
                sendBufferToClient(clientID, buffer, reliable);
        }
    }

    void Communicator::sendStringToClient(ClientID clientID, const std::string& string, bool reliable) {
        sendBufferToClient(clientID, Buffer(string.data(), string.size()), reliable);
    }

    void Communicator::sendStringToAllClients(const std::string& string, ClientID excludeClientID, bool reliable) {
        sendBufferToAllClients(Buffer(string.data(), string.size()), excludeClientID, reliable);
    }

    void Communicator::syncGameState(const ClientInfo& clientInfo) {
        Buffer buffer = SceneSynchronizer::serializeGameState(server.sceneManager);
        server.m_Communicator.sendBufferToClient(clientInfo.ID, buffer, true);
    }

    void Communicator::sendCustomPacket(const ClientID& clientID, const std::string& packetType, Buffer buffer, bool reliable) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CustomPacket) + buffer.size);
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CustomPacket);
        stream.writeString(packetType);
        stream.writeBuffer(buffer);
        sendBufferToClient(clientID, stream.getBuffer(), reliable);
    }
}
