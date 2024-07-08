#pragma once
#include "core/ServerExportDll.hpp"
#include "core/ClientInfo.hpp"
#include "serialization/Buffer.hpp"
#include <vector>

namespace TechEngine {
    class Server;
    class Communicator;

    class SERVER_DLL ServerAPI {
    private:
        inline static ServerAPI* instance;
        Communicator* communicator;
        Server* server;

    public:
        ServerAPI(Server* server, Communicator* communicator);

        static void sendCustomPacket(const ClientID& clientID, const std::string& packetType, Buffer buffer, bool reliable = true);

        static void sendCustomPacketToAllClients(const std::string& packetType, Buffer buffer, ClientID excludeClientID = 0, bool reliable = true);

        static std::vector<ClientID> getConnectedClients();
    };
}
