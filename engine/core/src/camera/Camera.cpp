#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <cstddef>
#include <vector>

namespace TechEngine {
    bool Camera::findMainCameraBelow(const Scene& scene, const Entity entity, Entity& camera) {
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

    bool Camera::findMainCamera(const Scene& scene, Entity& camera, bool& warnedNone) {
        std::size_t mainCount = 0;
        Entity firstFound;
        if (scene.isRegistered(componentTypeId<Camera>())) {
            auto query = scene.query<Write<>, Read<Camera>>();
            query.each([&](const Entity entity, const Camera& candidate) {
                if (!candidate.main) {
                    return;
                }
                if (mainCount == 0) {
                    firstFound = entity;
                }
                mainCount++;
            });
        }

        if (mainCount == 0) {
            if (!warnedNone) {
                TE_LOGGER_WARN("No entity has a Camera with main set");
                warnedNone = true;
            }
            return false;
        }

        camera = firstFound;
        if (mainCount > 1) {
            for (const Entity root: scene.getRoots()) {
                if (findMainCameraBelow(scene, root, camera)) {
                    break;
                }
            }
        }
        warnedNone = false;
        return true;
    }

    bool Camera::setMainCamera(Scene& scene, const Entity camera, std::vector<Entity>& cleared) {
        if (!scene.isRegistered(componentTypeId<Camera>()) || !scene.contains(camera) || !scene.hasComponent<Camera>(camera)) {
            return false;
        }

        auto query = scene.query<Write<Camera>, Read<>>();
        query.each([&](const Entity entity, Camera& candidate) {
            if (entity == camera) {
                candidate.main = true;
            } else if (candidate.main) {
                candidate.main = false;
                cleared.push_back(entity);
            }
        });
        return true;
    }

    std::size_t Camera::countMainCameras(const Scene& scene) {
        if (!scene.isRegistered(componentTypeId<Camera>())) {
            return 0;
        }
        std::size_t count = 0;
        auto query = scene.query<Write<>, Read<Camera>>();
        query.each([&](const Entity, const Camera& camera) {
            if (camera.main) {
                count++;
            }
        });
        return count;
    }
}
