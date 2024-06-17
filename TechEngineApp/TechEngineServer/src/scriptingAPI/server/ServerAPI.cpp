#include "ServerAPI.hpp"

#include "communicator/Communicator.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    ServerAPI::ServerAPI(Server* server, Communicator* communicator) : server(server), communicator(communicator) {
        instance = this;
    }

    void ServerAPI::sendCustomPacket(const ClientID& clientID, const std::string& packetType, Buffer buffer, bool reliable) {
        instance->communicator->sendCustomPacket(clientID, packetType, buffer, reliable);
    }

    void ServerAPI::sendCustomPacketToAllClients(const std::string& packetType, Buffer buffer, ClientID excludeClientID, bool reliable) {
        std::vector<ClientID> connectedClients = getConnectedClients();
        for (const auto& clientID: connectedClients) {
            if (clientID != excludeClientID) {
                sendCustomPacket(clientID, packetType, buffer, reliable);
            }
        }
    }

    std::vector<ClientID> ServerAPI::getConnectedClients() {
        std::vector<ClientID> connectedClients;
        for (const auto& client: instance->server->m_ConnectedClients) {
            connectedClients.push_back(client.second.ID);
        }
        return connectedClients;
    }
}
