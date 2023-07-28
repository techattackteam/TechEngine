#include "SceneHelper.hpp"
#include "components/DirectionalLightComponent.hpp"
namespace TechEngine {
    bool SceneHelper::findCameraComponent() {
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            if (gameObject->hasComponent<CameraComponent>()) {
                CameraComponent *cameraComponent = gameObject->getComponent<CameraComponent>();
                if (cameraComponent->isMainCamera()) {
                    mainCamera = cameraComponent;
                    return true;
                }
            }
        }
        return false;
    }

    bool SceneHelper::hasMainCamera() {
        if (mainCamera == nullptr) {
            return findCameraComponent();
        } else if (!mainCamera->isMainCamera()) {
            mainCamera = nullptr;
            return false;
        }
        return true;
    }

    void SceneHelper::clear() {
        mainCamera = nullptr;
        Scene::getInstance().clear();
    }
}