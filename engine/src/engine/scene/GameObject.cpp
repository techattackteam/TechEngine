#include "GameObject.hpp"

#include <utility>
#include "../event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "../event/EventDispatcher.hpp"
#include "../components/TransformComponent.hpp"

namespace Engine {
    GameObject::GameObject(std::string name, bool showInfoPanel) : name(std::move(name)) {
        this->showInfoPanel = showInfoPanel;
        addComponent<TransformComponent>(name);
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
    }

    void GameObject::fixUpdate() {
        for (auto element: components) {
            element.second->update();
        }
    }

    void GameObject::update() {

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
}
