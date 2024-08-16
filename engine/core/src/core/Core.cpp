#include "Core.hpp"

#include "Timer.hpp"

namespace TechEngine {
    void Core::init() {
        m_systemManager.registerSystem<Timer>();
        m_systemManager.getSystem<Timer>().init();
    }
}
