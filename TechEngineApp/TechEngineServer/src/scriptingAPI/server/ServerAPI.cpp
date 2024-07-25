#include "ServerAPI.hpp"

#include "communicator/Communicator.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    ServerAPI::ServerAPI(Server* server, Communicator* communicator) : server(server), communicator(communicator) {
        instance = this;
    }

    void ServerAPI::sendCustomPacket(const ClientInfo& clientInfo, const std::string& packetType, Buffer buffer, bool reliable) {
        instance->communicator->sendCustomPacket(clientInfo, packetType, buffer, reliable);
    }

    void ServerAPI::broadcastCustomPacket(const std::string& packetType, Buffer buffer, bool reliable, std::vector<ClientInfo> excludedClientsInfos) {
        std::vector<ClientInfo> connectedClients = getConnectedClients();
        for (const auto& clientInfo: connectedClients) {
            if (std::find(excludedClientsInfos.begin(), excludedClientsInfos.end(), clientInfo) == excludedClientsInfos.end()) {
                sendCustomPacket(clientInfo, packetType, buffer, reliable);
            }
        }
    }

    std::vector<ClientInfo> ServerAPI::getConnectedClients() {
        std::vector<ClientInfo> connectedClients;
        for (const auto& client: instance->server->m_connectedClients) {
            connectedClients.push_back(client.second);
        }
        return connectedClients;
    }
}
