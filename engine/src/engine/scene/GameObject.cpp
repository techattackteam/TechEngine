#include "GameObject.hpp"
#include "../event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "../event/EventDispatcher.hpp"
#include "../components/TransformComponent.hpp"

namespace Engine {
    GameObject::GameObject(bool showInfoPanel) {
        this->showInfoPanel = showInfoPanel;
        addComponent<TransformComponent>();
        EventDispatcher::getInstance().dispatch(new GameObjectCreateEvent(this));
    }

    void GameObject::update() {

    }

    void GameObject::showInfo() {
        for (auto element: components) {
            element.second->getInfo();
        }
    }

    glm::mat4 GameObject::getModelMatrix() {
        return ((TransformComponent *) getComponent<TransformComponent>())->getModelMatrix();
    }
}
