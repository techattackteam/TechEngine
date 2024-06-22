#include "Scene.hpp"

#include "components/render/MeshRendererComponent.hpp"
#include "core/UUID.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    Scene::Scene(SystemsRegistry& systemsRegistry) : systemsRegistry(systemsRegistry) {
        systemsRegistry.getSystem<EventDispatcher>().subscribe(MaterialUpdateEvent::eventType, [this](Event* event) {
            onMaterialUpdateEvent((MaterialUpdateEvent&)*event);
        });
    }

    Scene::~Scene() {
        clear();
    }

    GameObject& Scene::createGameObject(const std::string& name) {
        return registerGameObject(name, genGOTag());
    }


    GameObject& Scene::registerGameObject(const std::string& name, const std::string& tag) {
        GameObject* gameObject = new GameObject(name, tag, systemsRegistry);
        gameObjects.push_back(gameObject);
        return *gameObject;
    }

    void Scene::duplicateGameObject(GameObject* gameObject) {
        GameObject* newGameObject = new GameObject(gameObject, genGOTag(), systemsRegistry);
        gameObjects.push_back(newGameObject);
    }

    void Scene::deleteGameObject(GameObject* gameObject) {
        //delete gameObject and all its children
        if (gameObject->hasParent()) {
            gameObject->removeParent();
        }
        while (!gameObject->getChildren().empty()) {
            GameObject* child = gameObject->getChildren().begin()->second;
            deleteGameObject(child);
        }
        gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), gameObject), gameObjects.end());
        delete gameObject;
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
        UUID uuid;
        std::string uuidString = std::to_string(uuid);
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

    bool Scene::hasMainCamera() {
        for (GameObject* gameObject: gameObjects) {
            if (gameObject->hasComponent<CameraComponent>() && gameObject->getComponent<CameraComponent>()->isMainCamera()) {
                return true;
            }
        }
        return false;
    }

    CameraComponent* Scene::getMainCamera() {
        for (GameObject* gameObject: gameObjects) {
            if (gameObject->hasComponent<CameraComponent>() && gameObject->getComponent<CameraComponent>()->isMainCamera()) {
                return gameObject->getComponent<CameraComponent>();
            }
        }
        return nullptr;
    }

    void Scene::onMaterialUpdateEvent(MaterialUpdateEvent& event) {
        for (GameObject* gameObject: getGameObjects()) {
            if (gameObject->hasComponent<MeshRendererComponent>()) {
                MeshRendererComponent* meshRendererComponent = gameObject->getComponent<MeshRendererComponent>();
                if (meshRendererComponent->getMaterial().getName() == event.getMaterialName()) {
                    meshRendererComponent->paintMesh();
                }
            }
        }
    }
}
