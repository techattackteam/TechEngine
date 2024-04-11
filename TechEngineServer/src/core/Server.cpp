#include "Server.hpp"

namespace TechEngine {
    Server::Server() : AppCore() {
        timer.init();
    }

    Server::~Server() {
    }

    void Server::run() {
        while (running) {
            //TODO: move to Core module
        }
    }

    void Server::onUpdate() {
    }

    void Server::onFixedUpdate() {
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::Server();
}
