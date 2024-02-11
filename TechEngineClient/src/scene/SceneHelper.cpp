#include "SceneHelper.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    bool SceneHelper::findCameraComponent(Scene&scene) {
        for (GameObject* gameObject: scene.getGameObjects()) {
            if (gameObject->hasComponent<CameraComponent>()) {
                CameraComponent* cameraComponent = gameObject->getComponent<CameraComponent>();
                if (cameraComponent->isMainCamera()) {
                    mainCamera = cameraComponent;
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneHelper::hasMainCamera(Scene&scene) {
        if (mainCamera == nullptr) {
            return findCameraComponent(scene);
        } else if (!mainCamera->isMainCamera()) {
            mainCamera = nullptr;
            return false;
        }
        return true;
    }

    void SceneHelper::clear(Scene&scene) {
        mainCamera = nullptr;
    }

    void SceneHelper::changeMainCameraTo(CameraComponent* cameraComponent) {
        if (mainCamera != nullptr) {
            mainCamera->mainCamera = false;
        }
        mainCamera = cameraComponent;
        mainCamera->mainCamera = true;
    }
}
