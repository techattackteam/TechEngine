#include "Core.hpp"

#include "Timer.hpp"

namespace TechEngine {
    void Core::init() {
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.getSystem<Timer>().init();
    }
}
