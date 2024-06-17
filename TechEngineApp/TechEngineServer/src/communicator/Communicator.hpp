#pragma once
#include <string>

#include "core/ClientInfo.hpp"
#include "serialization/Buffer.hpp"

namespace TechEngine {
    class Server;

    class Communicator {
        Server& server;

    public:
        explicit Communicator(Server& server);

        template<typename T>
        void sendDataToClient(ClientID clientID, const T& data, bool reliable = true) {
            sendBufferToClient(clientID, Buffer(&data, sizeof(T)), reliable);
        }

        template<typename T>
        void sendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true) {
            sendBufferToAllClients(Buffer(&data, sizeof(T)), excludeClientID, reliable);
        }

        void sendBufferToClient(ClientID clientID, Buffer buffer, bool reliable);

        void sendBufferToAllClients(Buffer buffer, ClientID excludeClientID, bool reliable);

        void sendStringToClient(ClientID clientID, const std::string& string, bool reliable);

        void sendStringToAllClients(const std::string& string, ClientID excludeClientID, bool reliable);

        void syncGameState(const ClientInfo& clientInfo);

        void sendCustomPacket(const ClientID& clientID, const std::string& packetType, Buffer buffer, bool reliable);
    };
}
