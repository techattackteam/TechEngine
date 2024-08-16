#include "Server.hpp"

#include <iostream>

#include "core/Logger.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    Server::Server() : Core(), m_entry(m_systemManager) {
    }

    void Server::init() {
        Core::init();
        m_systemManager.registerSystem<Logger>("TechEngineServer");
        m_systemManager.getSystem<Logger>().init();
    }

    void Server::onStart() {
        m_systemManager.getSystem<Timer>().onStart();
        TE_LOGGER_INFO("Server started");
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
