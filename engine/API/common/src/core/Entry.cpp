#include "core/Entry.hpp"
#include "core/Timer.hpp"
#include "resources/Resources.hpp"

#include "scene/Scene.hpp"
#include "scene/ScenesManager.hpp"
#include "timer/Timer.hpp"
#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    void Entry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        Timer::init(&systemsRegistry->getSystem<TechEngine::Timer>());
        Scene::init(&systemsRegistry->getSystem<TechEngine::ScenesManager>().getActiveScene(), &systemsRegistry->getSystem<TechEngine::ResourcesManager>());
        Resources::init(&systemsRegistry->getSystem<TechEngine::ResourcesManager>());
    }

    void Entry::shutdown() {
        Timer::shutdown();
    }
}
