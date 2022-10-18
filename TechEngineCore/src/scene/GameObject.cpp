#include "GameObject.hpp"

#include <utility>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/EventDispatcher.hpp"
#include "components/TransformComponent.hpp"

namespace TechEngine {
    GameObject::GameObject(std::string name, bool showInfoPanel) : name(std::move(name)) {
        this->showInfoPanel = showInfoPanel;
        addComponent<TransformComponent>(this);
        TechEngineCore::EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
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

    void GameObject::showInfo() {

    }

    glm::mat4 GameObject::getModelMatrix() {
        return getComponent<TransformComponent>()->getModelMatrix();
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