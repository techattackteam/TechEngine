#include "SNetworkHandler.hpp"

#include <utility>
#include "SSocketHandler.hpp"
#include "SPacketHandler.hpp"
#include "SConnectionHandler.hpp"

namespace TechEngineServer {
    SNetworkHandler::SNetworkHandler(std::string ip, const short &port) : clients(), ip(std::move(ip)), port(port), NetworkHandler(new SSocketHandler(this),
                                                                                                                                   new SConnectionHandler(this),
                                                                                                                                   new SPacketHandler(this)) {
    }

    void SNetworkHandler::init() {
        ((SSocketHandler *) socketHandler)->init();
        //connectionHandler->init();
        ((SPacketHandler *) packetHandler)->init();
    }

    void SNetworkHandler::sendPacket(Packet *packet, Client *client) {
        packetHandler->sendPacket(client->UUID, packet, client->endpoint);
    }

    void SNetworkHandler::sendPacket(Packet *packet, const std::string &uuid) {
        sendPacket(packet, getClient(uuid));
    }

    std::unordered_map<std::string, Client *> &SNetworkHandler::getClients() {
        return clients;
    }

    Client *SNetworkHandler::getClient(const std::string &uuid) {
        return clients.at(uuid);
    }

    const std::string &SNetworkHandler::getIP() {
        return ip;
    }

    const short &SNetworkHandler::getPort() {
        return port;
    }

}