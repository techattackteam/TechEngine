#include "Scene.hpp"
#include "../event/EventDispatcher.hpp"

namespace Engine {
    Scene::Scene() {
        EventDispatcher::getInstance().subscribe(GameObjectCreateEvent::eventType, [this](Event *event) {
            onGOCreate((GameObjectCreateEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](Event *event) {
            onGODestroy((GameObjectDestroyEvent *) event);
        });
    }

    void Scene::onGOCreate(GameObjectCreateEvent *event) {
        gameObjects.emplace_back(event->getGameObject());
    }

    void Scene::onGODestroy(GameObjectDestroyEvent *event) {
        gameObjects.remove(event->getGameObject());
        delete (event->getGameObject());
    }
}
