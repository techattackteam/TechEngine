#include "Server.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    Server::Server() : Core(), m_entry(m_systemRegistry) {
    }

    void Server::init() {
        Core::init();
        m_systemRegistry.registerSystem<Logger>("TechEngineServer");
        m_systemRegistry.getSystem<Logger>().init();
    }

    void Server::onStart() {
        m_systemRegistry.getSystem<Timer>().onStart();
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
