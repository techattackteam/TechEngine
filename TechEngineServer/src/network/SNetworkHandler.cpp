#include "SNetworkHandler.hpp"
#include "SSocketHandler.hpp"
#include "SPacketHandler.hpp"
#include "SConnectionHandler.hpp"

namespace TechEngineServer {
    SNetworkHandler::SNetworkHandler(std::string ip, const short &port) : clients(), NetworkHandler(ip, port,
                                                                                                    new SSocketHandler(this),
                                                                                                    new SConnectionHandler(this),
                                                                                                    new SPacketHandler(this)) {
    }

    void SNetworkHandler::init() {
        ((SSocketHandler *) socketHandler)->init();
        //connectionHandler->init();
        ((SPacketHandler *) packetHandler)->init();
    }

    std::unordered_map<std::string, Client *> &SNetworkHandler::getClients() {
        return clients;
    }

    Client *SNetworkHandler::getClient(const std::string &uuid) {
        return clients.at(uuid);
    }

}