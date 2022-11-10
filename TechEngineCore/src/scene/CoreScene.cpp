#include <iostream>
#include <boost/uuid/uuid_io.hpp>
#include "CoreScene.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    CoreScene::CoreScene(const std::string &name) : name(name) {
        CoreScene::instance = this;
    }

    CoreScene::~CoreScene() {
        clear();
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

    std::string CoreScene::genGOTag() {
        boost::uuids::uuid uuid = goTagGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        return uuidString;
    }

    CoreScene &CoreScene::getInstance() {
        return *instance;
    }

    std::list<GameObject *> &CoreScene::getGameObjects() {
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

    void CoreScene::clear() {
        for (int i = 0; i < gameObjects.size(); i++) {
            //delete gameObjects.front();
        }
        gameObjects.clear();
    }
}
