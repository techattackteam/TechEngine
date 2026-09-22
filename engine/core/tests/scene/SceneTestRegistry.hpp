#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

namespace TechEngineTests {
    static void registerBuiltInSceneComponents(TechEngine::ComponentRegistry& registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
    }
}
