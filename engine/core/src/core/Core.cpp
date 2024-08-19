#include "Core.hpp"

#include "Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void Core::init() {
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.registerSystem<ScriptEngine>(m_systemRegistry);
        m_systemRegistry.getSystem<ScriptEngine>().init();
        m_systemRegistry.registerSystem<EventDispatcher>();
    }
}
