#include "Core.hpp"

#include "Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "scene/Scene.hpp"
#include "scene/TransformSystem.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void Core::init() {
        m_systemRegistry.registerSystem<EventDispatcher>();
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<ScriptEngine>(m_systemRegistry);
        m_systemRegistry.registerSystem<Scene>();
        m_systemRegistry.registerSystem<TransformSystem>(m_systemRegistry);
        m_systemRegistry.getSystem<EventDispatcher>().init();
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<ScriptEngine>().init();
        m_systemRegistry.getSystem<Scene>().init();
        m_systemRegistry.getSystem<TransformSystem>().init();
    }

    void Core::onStart() {
        m_systemRegistry.onStart();
    }

    void Core::onFixedUpdate() {
        m_systemRegistry.onFixedUpdate();
    }

    void Core::onUpdate() {
        m_systemRegistry.onUpdate();
    }

    void Core::onStop() {
        m_systemRegistry.onStop();
    }

    void Core::shutdown() {
        m_systemRegistry.onShutdown();
    }
}
