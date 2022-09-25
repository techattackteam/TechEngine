#pragma once

#include "tespph.hpp"
#include "core/Client.hpp"

namespace TechEngineServer {

    class SNetworkHandler : public NetworkHandler {
    private:
        std::unordered_map<std::string, Client *> clients;
        std::string ip;
        short port;
    public:
        SNetworkHandler(std::string ip, const short &port);

        void init() override;

        void sendPacket(Packet *packet, Client *client);

        void sendPacket(Packet *packet, const std::string &uuid);

        std::unordered_map<std::string, Client *> &getClients();

        Client *getClient(const std::string &uuid);

        const std::string &getIP();

        const short &getPort();

    };
}
