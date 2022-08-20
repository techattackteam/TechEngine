#pragma once

#include "tespph.hpp"
#include "core/Client.hpp"

namespace TechEngineServer {

    class SNetworkHandler : public NetworkHandler {
    private:
        std::unordered_map<std::string, Client *> clients;

    public:
        SNetworkHandler(std::string ip, const short &port);

        void init() override;

        std::unordered_map<std::string, Client *> &getClients();

        Client *getClient(const std::string &uuid);

    };
}
