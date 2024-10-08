#include "core/Timer.hpp"
#include "scene/ScenesManager.hpp"
#include "resources/ResourcesManager.hpp"

#include "common/include/core/Entry.hpp"
#include "common/include/resources/Resources.hpp"
#include "common/include/scene/Scene.hpp"
#include "common/include/timer/Timer.hpp"

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
