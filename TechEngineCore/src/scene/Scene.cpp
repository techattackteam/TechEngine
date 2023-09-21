#include <iostream>
#include <boost/uuid/uuid_io.hpp>
#include "Scene.hpp"
#include "event/EventDispatcher.hpp"
#include "components/light/DirectionalLightComponent.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"

namespace TechEngine {
    Scene::Scene() {
        Scene::instance = this;
        TechEngine::EventDispatcher::getInstance().subscribe(RequestDeleteGameObject::eventType, [this](Event *event) {
            onGameObjectDeleteRequest((RequestDeleteGameObject *) event);
        });
    }

    Scene::~Scene() {
        clear();
    }

    void Scene::registerGameObject(GameObject *gameObject) {
        registerGameObject(gameObject, genGOTag());
    }

    void Scene::registerGameObject(GameObject *gameObject, std::string tag) {
        gameObjects.emplace_back(gameObject);
        gameObject->setTag(tag);
        if (gameObject->hasComponent<DirectionalLightComponent>()) {
            lights.emplace_back(gameObject);
        }
    }

    void Scene::unregisterGameObject(GameObject *gameObject) {
        gameObjects.remove(gameObject);
        if (gameObject->hasComponent<DirectionalLightComponent>()) {
            lights.remove(gameObject);
        }

    }

    void Scene::update() {
        for (auto element: gameObjects) {
            element->update();
        }
        for (GameObject *gameObject: gameObjectsToDelete) {
            delete gameObject;
        }
        gameObjectsToDelete.clear();
    }

    void Scene::fixedUpdate() {
        for (auto element: gameObjects) {
            element->fixUpdate();
        }
    }

    void Scene::makeChildTo(GameObject *parent, GameObject *child) {
        if (getGameObjectByTag(child->getTag()) != nullptr) {
            gameObjects.remove(child);
        }
        parent->addChild(child);
    }

    std::string Scene::genGOTag() {
        boost::uuids::uuid uuid = goTagGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        return uuidString;
    }

    Scene &Scene::getInstance() {
        return *instance;
    }

    std::list<GameObject *> &Scene::getGameObjects() {
        return gameObjects;
    }

    std::list<GameObject *> Scene::getAllGameObjects() {
        std::list<GameObject *> allGameObjects;
        for (GameObject *gameObject: gameObjects) {
            std::vector<GameObject *> nodes_to_visit = {gameObject};
            while (!nodes_to_visit.empty()) {
                GameObject *child = nodes_to_visit.front();
                nodes_to_visit.erase(nodes_to_visit.begin());
                for (auto &child: child->getChildren()) {
                    nodes_to_visit.push_back(child.second);
                }
                allGameObjects.emplace_back(child);
            }
        }
        return allGameObjects;
    }

    std::list<GameObject *> &Scene::getLights() {
        return lights;
    }

    GameObject *Scene::getGameObject(const std::string &name) {
        for (GameObject *gameObject: gameObjects) {
            std::vector<GameObject *> nodes_to_visit = {gameObject};
            while (!nodes_to_visit.empty()) {
                GameObject *child = nodes_to_visit.front();
                nodes_to_visit.erase(nodes_to_visit.begin());
                for (auto &child: child->getChildren()) {
                    nodes_to_visit.push_back(child.second);
                }
                if (child->getTag() == name) {
                    return child;
                }
            }
        }
        return nullptr;
    }

    GameObject *Scene::getGameObjectByTag(const std::string &tag) {
        for (GameObject *gameObject: gameObjects) {
            std::vector<GameObject *> nodes_to_visit = {gameObject};
            while (!nodes_to_visit.empty()) {
                GameObject *child = nodes_to_visit.front();
                nodes_to_visit.erase(nodes_to_visit.begin());
                for (auto &child: child->getChildren()) {
                    nodes_to_visit.push_back(child.second);
                }
                if (child->getTag() == tag) {
                    return child;
                }
            }
        }
        return nullptr;
    }

    bool Scene::isLightingActive() const {
        return lights.size() != 0;
    }

    void Scene::clear() {
        while (!gameObjects.empty()) {
            delete gameObjects.front();
        }
        gameObjects.clear();
        lights.clear();
    }

    void Scene::onGameObjectDeleteRequest(RequestDeleteGameObject *event) {
        GameObject *gameObject = getGameObjectByTag(event->getTag());
        if (gameObject->hasParent()) {
            gameObject->removeParent();
        }
        gameObjectsToDelete.emplace_back(gameObject);
    }
}
