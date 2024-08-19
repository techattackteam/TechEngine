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
        Core::init();
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();

        m_systemRegistry.getSystem<EventDispatcher>().subscribe<TestEvent>([this](Event* event) {
            TE_LOGGER_TRACE("TestEvent received");
        });
    }

    void Client::onStart() {
        m_systemRegistry.getSystem<Timer>().onStart();
        m_systemRegistry.getSystem<ScriptEngine>().onStart();
        m_systemRegistry.getSystem<EventDispatcher>().dispatch(new TestEvent());
    }

    void Client::onFixedUpdate() {
        m_systemRegistry.getSystem<Timer>().onFixedUpdate();
        m_systemRegistry.getSystem<EventDispatcher>().onFixedUpdate();
        m_systemRegistry.getSystem<ScriptEngine>().onFixedUpdate();
    }

    void Client::onUpdate() {
        m_systemRegistry.getSystem<Timer>().onUpdate();
        m_systemRegistry.getSystem<EventDispatcher>().onUpdate();
        m_systemRegistry.getSystem<ScriptEngine>().onUpdate();
    }

    void Client::onStop() {
        m_systemRegistry.getSystem<Timer>().onStop();
        m_systemRegistry.getSystem<ScriptEngine>().onStop();
    }

    void Client::destroy() {
        m_systemRegistry.getSystem<Timer>().shutdown();
        m_systemRegistry.getSystem<EventDispatcher>().shutdown();
        m_systemRegistry.getSystem<ScriptEngine>().shutdown();
    }
}
