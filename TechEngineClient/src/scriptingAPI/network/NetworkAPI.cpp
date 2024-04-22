#include "NetworkAPI.hpp"

#include "network/NetworkEngine.hpp"

namespace TechEngine {
    NetworkAPI::NetworkAPI(NetworkEngine* networkEngine) {
        NetworkAPI::networkEngine = networkEngine;
    }

    void NetworkAPI::connectToServer(const std::string& ip, const std::string& port) {
        networkEngine->connectServer(ip, port);
    }

    void NetworkAPI::disconnectFromServer() {
        networkEngine->disconnectServer();
    }
}
