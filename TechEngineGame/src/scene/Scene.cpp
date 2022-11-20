#include "Scene.hpp"
#include "components/DirectionalLightComponent.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    Scene::Scene(const std::string &name) : CoreScene(name) {
        Scene::instance = this;
    }

    void Scene::registerGameObject(GameObject *gameObject) {
        CoreScene::registerGameObject(gameObject);
        if (gameObject->hasComponent<CameraComponent>()) {
            auto *cameraComponent = (CameraComponent *) gameObject->getComponent<CameraComponent>();
            if (cameraComponent->isMainCamera()) {
                mainCamera = cameraComponent;
            }
        } else if (gameObject->hasComponent<DirectionalLightComponent>()) {
            lights.emplace_back(gameObject);
        }
    }

    void Scene::unregisterGameObject(GameObject *gameObject) {
        gameObjects.remove(gameObject);
        if (gameObject->hasComponent<DirectionalLightComponent>()) {
            lights.remove(gameObject);
        }
    }

    std::list<GameObject *> Scene::getLights() {
        return lights;
    }

    bool Scene::findCameraComponent() {
        for (GameObject *gameObject: gameObjects) {
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

    bool Scene::hasMainCamera() {
        if (mainCamera == nullptr) {
            return findCameraComponent();
        } else if (!mainCamera->isMainCamera()) {
            mainCamera = nullptr;
            return false;
        }
        return true;
    }

    bool Scene::isLightingActive() const {
        return lights.size() != 0;
    }

    Scene &Scene::getInstance() {
        return *instance;
    }

    void Scene::clear() {
        CoreScene::clear();
        lights.clear();
    }

}