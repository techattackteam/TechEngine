#include "Client.hpp"
#include "core/Timer.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    Client::Client() : Core(), m_entry(m_systemRegistry) {
    }

    void Client::init() {
        Core::init();
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();
    }

    void Client::onStart() {
        m_systemRegistry.getSystem<Timer>().onStart();
        TE_LOGGER_INFO("Client started");
    }

    void Client::onFixedUpdate() {
        m_systemRegistry.getSystem<Timer>().onFixedUpdate();
    }

    void Client::onUpdate() {
        m_systemRegistry.getSystem<Timer>().onUpdate();
    }

    void Client::onStop() {
        m_systemRegistry.getSystem<Timer>().onStop();
    }

    void Client::destroy() {
        m_systemRegistry.getSystem<Timer>().shutdown();
    }
}
