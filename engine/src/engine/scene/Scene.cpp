#include "Scene.hpp"
#include "../event/EventDispatcher.hpp"

namespace Engine {
    Scene::Scene() {
        Scene::instance = this;
        EventDispatcher::getInstance().subscribe(GameObjectCreateEvent::eventType, [this](Event *event) {
            onGOCreate((GameObjectCreateEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](Event *event) {
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
        }
    }

    void Scene::onGODestroy(GameObjectDestroyEvent *event) {
        gameObjects.remove(event->getGameObject());
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

    bool Scene::hasMainCamera() {
        return mainCamera != nullptr;
    }
}
