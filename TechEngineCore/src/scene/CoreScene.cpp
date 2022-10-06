#include <iostream>
#include "CoreScene.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    CoreScene::CoreScene() {
        CoreScene::instance = this;
/*        TechEngineCore::EventDispatcher::getInstance().subscribe(GameObjectCreateEvent::eventType, [this](TechEngineCore::Event *event) {
            onGOCreate((GameObjectCreateEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](TechEngineCore::Event *event) {
            onGODestroy((GameObjectDestroyEvent *) event);
        });*/
    }

    void CoreScene::onGOCreate(GameObjectCreateEvent *event) {
        gameObjects.emplace_back(event->getGameObject());
    }

    void CoreScene::onGODestroy(GameObjectDestroyEvent *event) {
        gameObjects.remove(event->getGameObject());
    }

    void CoreScene::update() {
        for (auto element: gameObjects) {
            element->update();
        }
    }

    void CoreScene::fixedUpdate() {
        for (auto element: gameObjects) {
            element->fixUpdate();
        }
    }

    CoreScene &CoreScene::getInstance() {
        return *instance;
    }

    std::list<GameObject *> CoreScene::getGameObjects() {
        return gameObjects;
    }

    GameObject *CoreScene::getGameObject(std::string name) {
        for (GameObject *gameObject: gameObjects) {
            if (gameObject->getName() == name) {
                return gameObject;
            }
        }
        return nullptr;
    }
}
