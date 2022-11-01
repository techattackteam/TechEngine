#include <iostream>
#include "CoreScene.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    CoreScene::CoreScene(const std::string &name) : name(name) {
        CoreScene::instance = this;
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

    const std::string &CoreScene::getName() const {
        return name;
    }

    void CoreScene::setName(const std::string &name) {
        CoreScene::name = name;
    }
}
