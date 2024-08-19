#include "core/Entry.hpp"
#include "core/Timer.hpp"
#include "timer/Timer.hpp"

namespace TechEngineAPI {
    void Entry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Timer::init(&systemsRegistry->getSystem<TechEngine::Timer>());
    }

    void Entry::shutdown() {
        Timer::shutdown();
    }
}

