#pragma once
#include <string>

#include "core/ClientInfo.hpp"
#include "serialization/Buffer.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"

namespace TechEngine {
    class Server;

    class Communicator {
        Server& server;

    public:
        explicit Communicator(Server& server);

        template<typename T>
        void sendDataToClient(const ClientInfo& clientInfo, const T& data, bool reliable = true) {
            sendBufferToClient(clientInfo, Buffer(&data, sizeof(T)), reliable);
        }

        template<typename T>
        void sendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true) {
            broadcastBuffer(Buffer(&data, sizeof(T)), excludeClientID, reliable);
        }

        void sendBufferToClient(const ClientInfo& clientInfo, Buffer buffer, bool reliable);

        void broadcastBuffer(Buffer buffer, bool reliable, std::vector<ClientInfo> excludeClientsInfos = {});

        void sendNetworkID(const ClientInfo& clientInfo);

        void syncGameState(const ClientInfo& clientInfo);

        void sendNetworkObject(const ClientInfo& clientInfo, NetworkObject* networkObject);

        void broadcastNetworkObject(NetworkObject* networkObject);

        void syncNetworkObjects(const ClientInfo& clientInfo);

        void broadcastNetworkObjectDeleted(const std::string& uuid, std::vector<ClientInfo> excludedClientInfos = {});

        void deleteAllNetworkObjectsFromClient(const ClientInfo& clientInfo);

        void broadcastNetworkVariable(unsigned int owner, const std::string& uuid, const std::string& string, int value);

        void sendCustomPacket(const ClientInfo& clientInfo, const std::string& packetType, Buffer buffer, bool reliable);

    private:
        Buffer createNetworkObjectBuffer(const std::string& objectType, int owner, const std::string& networkUUID);

        Buffer createNetworkVariableBuffer(unsigned int owner, const std::string& uuid, const std::string& name, int value);
    };
}
