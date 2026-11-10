#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <edit/MaterialSlotRebuild.hpp>

#include <cstddef>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    void rebuildMaterialSlots(Scene& scene, ResourceCache& cache, const ResourceHandle<SceneBytes> sceneHandle, const std::span<const Entity> entities) {
        TE_CHECK(sceneHandle.valid(), "Material slots are rebuilt over a scene with no cache entry");
        std::vector<std::string> dropped;
        for (const Entity entity: entities) {
            if (!scene.contains(entity) || !scene.hasComponent<MeshRenderer>(entity)) {
                continue;
            }
            MeshRenderer& renderer = scene.getComponent<MeshRenderer>(entity);
            const Mesh* mesh = cache.resolve(renderer.mesh.handle);
            if (mesh == nullptr) {
                continue;
            }

            dropped.clear();
            renderer.rebuildSlots(*mesh, dropped);
            const std::string& name = scene.getComponent<Name>(entity).value;
            for (const std::string& slotName: dropped) {
                TE_LOGGER_WARN("Entity {0} '{1}' dropped the material of slot '{2}', which mesh {3:#018x} no longer has", entity, name, slotName, renderer.mesh.id.value());
            }

            for (MaterialSlot& slot: renderer.slots) {
                if (!slot.material.id.valid() || slot.material.handle.valid()) {
                    continue;
                }
                ResourceSlotId acquired;
                const std::error_code addError = cache.addDependency(sceneHandle, slot.material.id, acquired);
                if (addError) {
                    TE_LOGGER_WARN("Entity {0} '{1}' cannot hold material {2:#018x} for slot '{3}': {4}", entity, name, slot.material.id.value(), slot.name, addError.message());
                    continue;
                }
                slot.material.handle = ResourceHandle<Material>{acquired.index, acquired.generation};
            }
        }
    }

    void rebuildMaterialSlots(Scene& scene, ResourceCache& cache, const ResourceHandle<SceneBytes> sceneHandle) {
        std::vector<Entity> entities = scene.getRoots();
        for (std::size_t index = 0; index < entities.size(); index++) {
            const std::vector<Entity> children = scene.getChildren(entities[index]);
            entities.insert(entities.end(), children.begin(), children.end());
        }
        rebuildMaterialSlots(scene, cache, sceneHandle, entities);
    }
}
