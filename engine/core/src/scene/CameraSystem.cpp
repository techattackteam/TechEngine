#include "CameraSystem.hpp"

#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    CameraSystem::CameraSystem(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void CameraSystem::setFov(Entity entity, float fov) {
        auto& camera = m_systemsRegistry.getSystem<Scene>().getComponent<Camera>(entity);
        camera.fov = fov;
    }

    void CameraSystem::setNear(Entity entity, float nearPlane) {
        auto& camera = m_systemsRegistry.getSystem<Scene>().getComponent<Camera>(entity);
        camera.nearPlane = nearPlane;
    }

    void CameraSystem::setFar(Entity entity, float farPlane) {
        auto& camera = m_systemsRegistry.getSystem<Scene>().getComponent<Camera>(entity);
        camera.farPlane = farPlane;
    }

    void CameraSystem::setMainCamera(Entity entity) {
        auto& scene = m_systemsRegistry.getSystem<Scene>();
        removeMainCamera();
        scene.getComponent<Camera>(entity).mainCamera = true;
    }

    bool CameraSystem::hasMainCamera() {
        auto& scene = m_systemsRegistry.getSystem<Scene>();
        for (Camera& camera: scene.getComponents<Camera>()) {
            if (camera.isMainCamera()) {
                return true;
            }
        }
        return false;
    }

    Camera& CameraSystem::getMainCamera() {
        auto& scene = m_systemsRegistry.getSystem<Scene>();
        for (Camera& camera: scene.getComponents<Camera>()) {
            if (camera.isMainCamera()) {
                TE_LOGGER_INFO("CameraSystem: Main Camera fov: {0}, Address: {1}", camera.getFov(), static_cast<void*>(&camera));
                return camera;
            }
        }
        TE_LOGGER_WARN("No main camera found.");
    }


    void CameraSystem::removeMainCamera() {
        auto& scene = m_systemsRegistry.getSystem<Scene>();
        for (Camera& camera: scene.getComponents<Camera>()) {
            if (camera.isMainCamera()) {
                camera.mainCamera = false;
                break;
            }
        }
    }
}
