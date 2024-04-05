#include <boost/uuid/uuid_io.hpp>
#include "Scene.hpp"

#include "eventSystem/EventDispatcher.hpp"
//#include "events/gameObjects/RequestDeleteGameObject.hpp"

namespace TechEngine {
    Scene::Scene() {
        /*EventDispatcher::getInstance().subscribe(RequestDeleteGameObject::eventType, [this](Event* event) {
            onGameObjectDeleteRequest((RequestDeleteGameObject*)event);
        });*/
    }

    Scene::~Scene() {
        clear();
    }


    void Scene::deleteGameObject(GameObject* gameObject) {
        for (auto it = gameObjects.begin(); it != gameObjects.end(); it++) {
            if (*it == gameObject) {
                gameObjects.erase(it);
                delete gameObject;
                return;
            }
        }
    }

    void Scene::update() {
        for (GameObject* gameObject: gameObjects) {
            gameObject->update();
        }
        for (GameObject* gameObject: gameObjectsToDelete) {
            deleteGameObject(gameObject);
        }
        gameObjectsToDelete.clear();
    }

    void Scene::fixedUpdate() {
        for (GameObject* element: gameObjects) {
            element->fixUpdate();
        }
    }

    void Scene::makeChildTo(GameObject* parent, GameObject* child) {
        parent->addChild(child);
    }

    std::string Scene::genGOTag() {
        boost::uuids::uuid uuid = goTagGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        return uuidString;
    }

    std::vector<GameObject*> Scene::getGameObjects() {
        std::vector<GameObject*> gameObjects;
        for (GameObject* gameObject: this->gameObjects) {
            gameObjects.emplace_back(gameObject);
        }
        return gameObjects;
    }

    std::vector<GameObject*> Scene::getLights() {
        return lights;
    }

    GameObject* Scene::getGameObject(const std::string& name) {
        for (GameObject* gameObject: gameObjects) {
            if (gameObject->getName() == name) {
                return gameObject;
            }
        }
        return nullptr;
    }

    GameObject* Scene::getGameObjectByTag(const std::string& tag) {
        for (GameObject* gameObject: gameObjects) {
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

    void Scene::onGameObjectDeleteRequest() {
        /*GameObject* gameObject = getGameObjectByTag(event->getTag());
        if (gameObject->hasParent()) {
            gameObject->removeParent();
        }
        gameObjectsToDelete.emplace_back(gameObject);*/
    }
}
