#include "common/include/core/Entry.hpp"

#include "core/Timer.hpp"
#include "scene/ScenesManager.hpp"
#include "resources/ResourcesManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "audio/AudioSystem.hpp"
#include "components/Components.hpp"

#include "common/include/resources/Resources.hpp"
#include "common/include/scene/Scene.hpp"
#include "common/include/timer/Timer.hpp"


namespace TechEngineAPI {
    void Entry::init(TechEngine::SystemsRegistry* systemsRegistry) {
        TechEngine::ComponentType<TechEngine::Tag>::get();
        TechEngine::ComponentType<TechEngine::Transform>::get();
        TechEngine::ComponentType<TechEngine::Camera>::get();
        TechEngine::ComponentType<TechEngine::MeshRenderer>::get();
        Timer::init(&systemsRegistry->getSystem<TechEngine::Timer>());
        Resources::init(&systemsRegistry->getSystem<TechEngine::ResourcesManager>());
        Scene::init(&systemsRegistry->getSystem<TechEngine::ScenesManager>().getActiveScene(),
                    &systemsRegistry->getSystem<TechEngine::ResourcesManager>(),
                    &systemsRegistry->getSystem<TechEngine::PhysicsEngine>(),
                    &systemsRegistry->getSystem<TechEngine::AudioSystem>());
    }

    void Entry::shutdown() {
        Timer::shutdown();
    }
}
