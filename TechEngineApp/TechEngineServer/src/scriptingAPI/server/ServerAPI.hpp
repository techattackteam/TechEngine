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

        static void sendCustomPacket(const ClientInfo& clientInfo, const std::string& packetType, Buffer buffer, bool reliable = true);

        static void broadcastCustomPacket(const std::string& packetType, Buffer buffer, bool reliable = true, std::vector<ClientInfo> excludedClientsInfos = {});

        static std::vector<ClientInfo> getConnectedClients();
    };
}
