#include <iostream>
#include "Scene.hpp"
#include "../components/DirectionalLightComponent.hpp"

namespace TechEngine {
    Scene::Scene() {
        Scene::instance = this;
        TechEngineCore::EventDispatcher::getInstance().subscribe(GameObjectCreateEvent::eventType, [this](TechEngineCore::Event *event) {
            onGOCreate((GameObjectCreateEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](TechEngineCore::Event *event) {
            onGODestroy((GameObjectDestroyEvent *) event);
        });
    }

    void Scene::onGOCreate(GameObjectCreateEvent *event) {
        gameObjects.emplace_back(event->getGameObject());
        if (event->getGameObject()->hasComponent<CameraComponent>()) {
            auto *cameraComponent = (CameraComponent *) event->getGameObject()->getComponent<CameraComponent>();
            if (cameraComponent->isMainCamera()) {
                mainCamera = cameraComponent;
            }
        } else if (event->getGameObject()->hasComponent<DirectionalLightComponent>()) {
            lights.emplace_back(event->getGameObject());
        }
    }

    void Scene::onGODestroy(GameObjectDestroyEvent *event) {
        gameObjects.remove(event->getGameObject());
        if (event->getGameObject()->hasComponent<DirectionalLightComponent>()) {
            lights.remove(event->getGameObject());
        }
        delete (event->getGameObject());
    }

    void Scene::update() {
        for (auto element: gameObjects) {
            element->update();
        }
    }

    void Scene::fixedUpdate() {
        for (auto element: gameObjects) {
            element->fixUpdate();
        }
    }

    Scene &Scene::getInstance() {
        return *instance;
    }

    std::list<GameObject *> Scene::getGameObjects() {
        return gameObjects;
    }

    std::list<GameObject *> Scene::getLights() {
        return lights;
    }

    bool Scene::hasMainCamera() {
        return mainCamera != nullptr;
    }

    bool Scene::isLightingActive() const {
        return lights.size() != 0;
    }


}
