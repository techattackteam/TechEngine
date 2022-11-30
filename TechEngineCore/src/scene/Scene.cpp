#include <iostream>
#include <boost/uuid/uuid_io.hpp>
#include <utility>
#include "Scene.hpp"
#include "event/EventDispatcher.hpp"
#include "components/DirectionalLightComponent.hpp"

namespace TechEngine {
    Scene::Scene(std::string name) : name(std::move(name)) {
        Scene::instance = this;
    }

    Scene::~Scene() {
        clear();
    }

    void Scene::registerGameObject(GameObject *gameObject) {
        gameObjects.emplace_back(gameObject);
        gameObject->setTag(genGOTag());
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
    }

    void Scene::fixedUpdate() {
        for (auto element: gameObjects) {
            element->fixUpdate();
        }
    }

    void Scene::makeChildTo(GameObject *parent, GameObject *child) {
        if (getGameObject(child->getName()) != nullptr) {
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

    std::list<GameObject *> &Scene::getLights() {
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

    bool Scene::isLightingActive() const {
        return lights.size() != 0;
    }

    const std::string &Scene::getName() const {
        return name;
    }

    void Scene::setName(const std::string &name) {
        Scene::name = name;
    }

    void Scene::clear() {
        for (int i = 0; i < gameObjects.size(); i++) {
            //delete gameObjects.front();
        }
        gameObjects.clear();
    }
}
