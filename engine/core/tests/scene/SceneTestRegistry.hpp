#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <concepts>

namespace TechEngineTests {
    template<typename Component>
    concept WritableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) {
        { scene.getComponent<Component>(entity) } -> std::same_as<Component&>;
    };

    template<typename Component>
    concept AddableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) { scene.addComponent<Component>(entity); };

    template<typename Component>
    concept AddableToPendingSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::PendingEntity entity) { scene.addComponent<Component>(entity); };

    template<typename Component>
    concept RemovableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) { scene.removeComponent<Component>(entity); };

    template<typename Component>
    concept RemovableFromPendingSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::PendingEntity entity) { scene.removeComponent<Component>(entity); };

    static void registerBuiltInSceneComponents(TechEngine::ComponentRegistry& registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        registry.registerComponent<TechEngine::Name>(TechEngine::Name::tag);
    }
}
