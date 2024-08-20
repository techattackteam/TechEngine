#include "Client.hpp"

#include "core/Timer.hpp"
#include "core/Logger.hpp"
#include "events/TestEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    Client::Client() : Core(), m_entry(m_systemRegistry) {
    }

    void Client::init() {
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();
        Core::init();
    }

    void Client::onStart() {
        Core::onStart();
    }

    void Client::onFixedUpdate() {
        Core::onFixedUpdate();
    }

    void Client::onUpdate() {
        Core::onUpdate();
    }

    void Client::onStop() {
        Core::onStop();
    }

    void Client::shutdown() {
        Core::shutdown();
    }
}
