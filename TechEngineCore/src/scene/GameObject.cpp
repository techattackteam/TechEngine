#include "GameObject.hpp"

#include <utility>
#include "event/EventDispatcher.hpp"
#include "components/TransformComponent.hpp"
#include "CoreScene.hpp"
#include <iostream>

namespace TechEngine {
    GameObject::GameObject(std::string name) : name(std::move(name)) {
        addComponent<TransformComponent>(this);
        TechEngineCore::EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
        //CoreScene::getInstance().getGameObjects().push_back(this);
        std::cout << this->name << " is created" << std::endl;
    }

    GameObject::~GameObject() {
        //TechEngineCore::EventDispatcher::getInstance().dispatch(new GameObjectDestroyEvent(this));
        CoreScene::getInstance().getGameObjects().remove(this);
        std::cout << name << " is destroyed" << std::endl;
    }

    void GameObject::fixUpdate() {
        for (auto element: components) {
            element.second->fixedUpdate();
        }
    }

    void GameObject::update() {
        for (auto element: components) {
            element.second->update();
        }
    }

    glm::mat4 GameObject::getModelMatrix() {
        return getTransform().getModelMatrix();
    }

    glm::mat4 GameObject::getModelMatrixInterpolated() {
        return getTransform().getModelMatrix();
    }

    std::string GameObject::getName() {
        return name;
    }

    TransformComponent &GameObject::getTransform() {
        return *getComponent<TransformComponent>();
    }

    std::unordered_map<ComponentName, Component *> *GameObject::getComponents() {
        return &components;
    }
}