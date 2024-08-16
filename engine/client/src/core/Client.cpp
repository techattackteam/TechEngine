#include "Client.hpp"
#include "core/Timer.hpp"
#include "core/Logger.hpp"
#include <iostream>

namespace TechEngine {
    Client::Client() : Core(), m_entry(m_systemManager) {
    }

    void Client::init() {
        Core::init();
        m_systemManager.registerSystem<Logger>("TechEngineClient");
        m_systemManager.getSystem<Logger>().init();
    }

    void Client::onStart() {
        m_systemManager.getSystem<Timer>().onStart();
        TE_LOGGER_INFO("Client started");
    }

    void Client::onFixedUpdate() {
        m_systemManager.getSystem<Timer>().onFixedUpdate();
    }

    void Client::onUpdate() {
        m_systemManager.getSystem<Timer>().onUpdate();
    }

    void Client::onStop() {
        m_systemManager.getSystem<Timer>().onStop();
    }

    void Client::destroy() {
        m_systemManager.getSystem<Timer>().destroy();
    }
}
