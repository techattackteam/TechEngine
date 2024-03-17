#include "Component.hpp"

#include <utility>
#include "TransformComponent.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    Component::Component(GameObject* gameObject, std::string name) : gameObject(gameObject), name(std::move(name)) {
    }

    Component::~Component() {
    }

    void Component::fixedUpdate() {
    }

    void Component::update() {
    }

    TransformComponent& Component::getTransform() {
        return gameObject->getTransform();
    }

    GameObject* Component::getGameObject() {
        return gameObject;
    }
}
