#include "Client.hpp"
#include "core/Timer.hpp"

#include <iostream>

namespace TechEngine {
    Client::Client() : Core(), m_entry(m_systemManager) {
    }

    void Client::init() {
        Core::init();
    }

    void Client::onStart() {
        m_systemManager.getSystem<Timer>().onStart();
    }

    void Client::onFixedUpdate() {
        m_systemManager.getSystem<Timer>().onFixedUpdate();
    }

    void Client::onUpdate() {
        m_systemManager.getSystem<Timer>().onUpdate();
        std::cout << m_systemManager.getSystem<Timer>().getTime() << std::endl;
    }

    void Client::onStop() {
        m_systemManager.getSystem<Timer>().onStop();
    }

    void Client::destroy() {
        m_systemManager.getSystem<Timer>().destroy();
    }
}
