#include "Communicator.hpp"

#include "core/Logger.hpp"
#include "core/Server.hpp"
#include "network/NetworkObjectsManager.hpp"
#include "network/SceneSynchronizer.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"

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

    Buffer Communicator::createNetworkObjectBuffer(const std::string& objectType, unsigned int owner, const std::string& networkUUID) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::CreateNetworkObject) + sizeof(unsigned int) + sizeof(networkUUID));
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::CreateNetworkObject);
        stream.writeString(objectType);
        stream.writeRaw<unsigned int>(owner);
        stream.writeString(networkUUID);
        return scratchBuffer;
    }

    void Communicator::sendNetworkObject(const ClientInfo& clientInfo, const std::string& objectType, NetworkObject* networkObject) {
        Buffer scratchBuffer = createNetworkObjectBuffer(objectType, networkObject->getOwner(), networkObject->getNetworkUUID());
        sendBufferToClient(clientInfo.ID, scratchBuffer, true);
    }

    void Communicator::sendNetworkObjectToAllClients(const std::string& objectType, NetworkObject* networkObject) {
        Buffer scratchBuffer = createNetworkObjectBuffer(objectType, networkObject->getOwner(), networkObject->getNetworkUUID());
        sendBufferToAllClients(scratchBuffer, 0, true);
    }

    void Communicator::syncNetworkObjects(const ClientInfo& clientInfo) {
        for (NetworkObject* networObject: server.systemsRegistry.getSystem<NetworkObjectsManager>().getNetworkObjects()) {
            sendNetworkObject(clientInfo, typeid(*networObject).name(), networObject);
        }
    }

    void Communicator::syncGameState(const ClientInfo& clientInfo) {
        Buffer buffer = SceneSynchronizer::serializeGameState(server.systemsRegistry.getSystem<SceneManager>());
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

    void Communicator::sendNetworkObjectDeleted(const std::string& uuid, ClientID excludeClientID) {
        Buffer scratchBuffer;
        scratchBuffer.allocate(sizeof(PacketType::DeleteNetworkObject) + uuid.size());
        BufferStreamWriter stream(scratchBuffer);
        stream.writeRaw<PacketType>(PacketType::DeleteNetworkObject);
        stream.writeString(uuid);
        sendBufferToAllClients(scratchBuffer, excludeClientID, true);
    }

    void Communicator::deleteAllNetworkObjectFromClient(uint32 id) {
        for (NetworkObject* networkObject: server.systemsRegistry.getSystem<NetworkObjectsManager>().getNetworkObjects()) {
            if (networkObject->getOwner() == id) {
                sendNetworkObjectDeleted(networkObject->getNetworkUUID(), id);
                server.systemsRegistry.getSystem<NetworkObjectsManager>().deleteNetworkObject(networkObject->getNetworkUUID());
            }
        }
    }
}
