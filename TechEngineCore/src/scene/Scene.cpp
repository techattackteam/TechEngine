#include <iostream>
#include <boost/uuid/uuid_io.hpp>
#include "Scene.hpp"
#include "event/EventDispatcher.hpp"
#include "components/light/DirectionalLightComponent.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"

namespace TechEngine {
    Scene::Scene() {
        EventDispatcher::getInstance().subscribe(RequestDeleteGameObject::eventType, [this](Event *event) {
            onGameObjectDeleteRequest((RequestDeleteGameObject *) event);
        });
    }

    Scene::~Scene() {
        clear();
    }


    void Scene::deleteGameObject(GameObject *gameObject) {
        gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), gameObject), gameObjects.end());
    }

    void Scene::update() {
        for (GameObject *gameObject: gameObjects) {
            gameObject->update();
        }
        for (GameObject *gameObject: gameObjectsToDelete) {
            deleteGameObject(gameObject);
        }
        gameObjectsToDelete.clear();
    }

    void Scene::fixedUpdate() {
        for (GameObject *element: gameObjects) {
            element->fixUpdate();
        }
    }

    void Scene::makeChildTo(GameObject *parent, GameObject *child) {
        parent->addChild(child);
    }

    std::string Scene::genGOTag() {
        boost::uuids::uuid uuid = goTagGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        return uuidString;
    }

    std::vector<GameObject *> Scene::getGameObjects() {
        std::vector<GameObject *> gameObjects;
        for (GameObject *gameObject: this->gameObjects) {
            gameObjects.emplace_back(gameObject);
        }
        return gameObjects;
    }

    std::vector<GameObject *> Scene::getLights() {
        return lights;
    }

    GameObject *Scene::getGameObject(const std::string &name) {
        for (GameObject *gameObject: gameObjects) {
            if (gameObject->getName() == name) {
                return gameObject;
            }
        }
        return nullptr;
    }

    GameObject *Scene::getGameObjectByTag(const std::string &tag) {
        for (GameObject *gameObject: gameObjects) {
            if (gameObject->getTag() == tag) {
                return gameObject;
            }
        }
        return nullptr;
    }

    bool Scene::isLightingActive() const {
        return lights.size() != 0;
    }

    void Scene::clear() {
        gameObjects.clear();
        lights.clear();
        gameObjectsToDelete.clear();
    }

    void Scene::onGameObjectDeleteRequest(RequestDeleteGameObject *event) {
        GameObject *gameObject = getGameObjectByTag(event->getTag());
        if (gameObject->hasParent()) {
            gameObject->removeParent();
        }
        gameObjectsToDelete.emplace_back(gameObject);
    }
}
