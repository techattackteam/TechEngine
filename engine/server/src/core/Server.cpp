#include "Server.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    Server::Server() : Core(), m_entry(m_systemRegistry) {
    }

    void Server::init() {
        m_systemRegistry.registerSystem<Logger>("TechEngineServer");
        m_systemRegistry.getSystem<Logger>().init();
        Core::init();
    }

    void Server::onStart() {
        Core::onStart();
        TE_LOGGER_ERROR("Server onStart");
    }

    void Server::onFixedUpdate() {
        Core::onFixedUpdate();
    }

    void Server::onUpdate() {
        Core::onUpdate();
    }

    void Server::onStop() {
        Core::onStop();
    }

    void Server::shutdown() {
        Core::shutdown();
    }
}
