#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <vector>

namespace TechEngine {
    static bool findMainCameraBelow(const Scene& scene, const Entity entity, Entity& camera) {
        if (scene.hasComponent<Camera>(entity) && scene.getComponent<Camera>(entity).main) {
            camera = entity;
            return true;
        }
        for (const Entity child: scene.getChildren(entity)) {
            if (findMainCameraBelow(scene, child, camera)) {
                return true;
            }
        }
        return false;
    }

    bool findMainCamera(const Scene& scene, Entity& camera, bool& warnedNone) {
        for (const Entity root: scene.getRoots()) {
            if (findMainCameraBelow(scene, root, camera)) {
                warnedNone = false;
                return true;
            }
        }
        if (!warnedNone) {
            TE_LOGGER_WARN("No entity has a Camera with main set, so the view only clears");
            warnedNone = true;
        }
        return false;
    }
}
