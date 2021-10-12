#include "GameObject.hpp"

#include <utility>
#include "../event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "../event/EventDispatcher.hpp"

namespace Engine {
    GameObject::GameObject(std::string name, bool showInfoPanel) : name(std::move(name)) {
        this->showInfoPanel = showInfoPanel;
        addComponent<TransformComponent>();
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
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
        for (auto element: components) {
            element.second->getInfo();
        }
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
}
