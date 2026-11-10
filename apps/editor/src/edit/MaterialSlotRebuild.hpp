#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <span>

namespace TechEngine {
    class ResourceCache;
    class Scene;
    struct SceneBytes;

    // The scene's entry must be Ready. An entity without a MeshRenderer, or whose mesh is not Ready, is skipped. A new
    // default is held by the scene until the scene is released, as an asset field set's asset is. A default that cannot
    // be held is logged and keeps its id with an empty handle, and the other slots and entities are still rebuilt.
    void rebuildMaterialSlots(Scene& scene, ResourceCache& cache, ResourceHandle<SceneBytes> sceneHandle, std::span<const Entity> entities);

    // Over every entity in the scene.
    void rebuildMaterialSlots(Scene& scene, ResourceCache& cache, ResourceHandle<SceneBytes> sceneHandle);
}
