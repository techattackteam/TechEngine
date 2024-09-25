#include "core/Entry.hpp"
#include "core/Timer.hpp"

#include "scene/Scene.hpp"
#include "scene/ScenesManager.hpp"
#include "timer/Timer.hpp"

namespace TechEngineAPI {
    void Entry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Timer::init(&systemsRegistry->getSystem<TechEngine::Timer>());
        Scene::init(&systemsRegistry->getSystem<TechEngine::ScenesManager>().getActiveScene());
    }

    void Entry::shutdown() {
        Timer::shutdown();
    }
}
