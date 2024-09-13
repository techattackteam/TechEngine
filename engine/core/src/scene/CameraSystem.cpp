#include "CameraSystem.hpp"

#include "Scene.hpp"
#include "ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    CameraSystem::CameraSystem(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void CameraSystem::setFov(Entity entity, float fov) {
        auto& camera = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Camera>(entity);
        camera.fov.x = fov;
    }

    void CameraSystem::setNear(Entity entity, float nearPlane) {
        auto& camera = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Camera>(entity);
        camera.nearPlane = nearPlane;
    }

    void CameraSystem::setFar(Entity entity, float farPlane) {
        auto& camera = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Camera>(entity);
        camera.farPlane = farPlane;
    }

    void CameraSystem::setMainCamera(Camera& camera) {
        removeMainCamera();
        camera.mainCamera = true;
    }

    bool CameraSystem::hasMainCamera() {
        /*auto& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        auto matchingArchetypes = scene.queryArchetypes({ComponentType::get<Camera>()});
        if (matchingArchetypes.empty()) {
            return false;
        }
        for (auto& archetype: matchingArchetypes) {
            auto& cameras = archetype.getComponentArray<Camera>();
            for (const auto& camera: cameras) {
                if (camera.isMainCamera()) {
                    return true;
                }
            }
        }*/

        return false;
    }

    /*Camera& CameraSystem::getMainCamera() {
        /*auto& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        for (Camera& camera: scene.getComponents<Camera>()) {
            if (camera.isMainCamera()) {
                return camera;
            }
        }#1#
        //return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Camera>(m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByTag(Tag("Main Camera", "")));
    }*/

    void CameraSystem::removeMainCamera() {
        /*auto& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        auto matchingArchetypes = scene.queryArchetypes({ComponentType::get<Camera>()});
        for (auto& archetype: matchingArchetypes) {
            auto& cameras = archetype.getComponentArray<Camera>();
            for (auto& camera: cameras) {
                if (camera.isMainCamera()) {
                    camera.mainCamera = false;
                    break;
                }
            }
        }*/
    }
}
