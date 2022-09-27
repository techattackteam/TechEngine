#include "GameObject.hpp"

#include <utility>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/EventDispatcher.hpp"
#include "components/Transform.hpp"

namespace TechEngine {
    GameObject::GameObject(std::string name, bool showInfoPanel) : name(std::move(name)) {
        this->showInfoPanel = showInfoPanel;
        addComponent<Transform>(this);
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
        getTransform().getInfo();
        for (auto element: components) {
            if (element.first == Transform::getName()) {
                continue;
            }
            element.second->getInfo();
        }
    }

    glm::mat4 GameObject::getModelMatrix() {
        return getComponent<Transform>()->getModelMatrix();
    }

    std::string GameObject::getName() {
        return name;
    }

    Transform &GameObject::getTransform() {
        return *getComponent<Transform>();
    }

}
