#include "Timer.hpp"
#include "core/Timer.hpp"

namespace TechEngineAPI {
    void Timer::init(TechEngine::Timer* timer) {
        Timer::timer = timer;
    }

    void Timer::shutdown() {
    }

    double Timer::getDeltaTime() {
        return timer->getDeltaTime();
    }
}
