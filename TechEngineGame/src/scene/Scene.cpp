#include "Scene.hpp"
#include "components/DirectionalLightComponent.hpp"

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
        CoreScene::onGOCreate(event);
        if (event->getGameObject()->hasComponent<CameraComponent>()) {
            auto *cameraComponent = (CameraComponent *) event->getGameObject()->getComponent<CameraComponent>();
            if (cameraComponent->isMainCamera()) {
                mainCamera = cameraComponent;
            }
        } else if (event->getGameObject()->hasComponent<DirectionalLightComponent>()) {
            lights.emplace_back(event->getGameObject());
        }
        //Hack
        event->getGameObject()->removeComponent<TransformComponent>();
        event->getGameObject()->addComponent<TransformComponent>(event->getGameObject());
    }

    void Scene::onGODestroy(GameObjectDestroyEvent *event) {
        CoreScene::onGODestroy(event);
        if (event->getGameObject()->hasComponent<DirectionalLightComponent>()) {
            lights.remove(event->getGameObject());
        }
        delete (event->getGameObject());
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

    Scene &Scene::getInstance() {
        return *instance;
    }
}