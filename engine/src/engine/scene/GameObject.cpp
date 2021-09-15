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

    template<typename C>
    void GameObject::addComponent() {
        C *component = new C();
        if (!components.contains(component->getName())) {
            components[component->getName()] = component;
        }
    }

    template<typename C, typename A>
    void GameObject::addComponent(A args...) {
        C *component = new C(args);
        if (!components.contains(component->getName())) {
            components[component->getName()] = component;
        }
    }

    void GameObject::removeComponent(std::string &name) {
        //components.erase(std::remove_if(components.begin(), components.end(), name), components.end());
    }

    template<typename C>
    bool GameObject::hasComponent() {
        C component = C();
        return components.contains(component->getName());
    }

    template<typename C>
    bool GameObject::getComponent() {
        C component = C();
        return components[component.getName()];
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
