#include "Server.hpp"

#include <iostream>

namespace TechEngine {
    Server::Server() : Core(), m_entry(m_systemManager) {
    }

    void Server::init() {
        Core::init();
    }

    void Server::onStart() {
    }

    void Server::onFixedUpdate() {
    }

    void Server::onUpdate() {
    }

    void Server::onStop() {
    }

    void Server::destroy() {
    }
}
