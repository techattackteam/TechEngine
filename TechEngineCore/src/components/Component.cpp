#include "Component.hpp"

#include "TransformComponent.hpp"
#include "scene/GameObject.hpp"

#include <utility>

namespace TechEngine {
    Component::Component(GameObject* gameObject, std::string name) : gameObject(gameObject), name(std::move(name)) {
    }

    Component::~Component() {
    }

    void Component::fixedUpdate() {
    }

    void Component::update() {
    }

    void Component::setGameObject(const GameObject& gameObject) {
        *this->gameObject = gameObject;
    }

    void Component::Serialize(StreamWriter* stream, const Component& component) {
        stream->writeString(component.name);
    }

    void Component::Deserialize(StreamReader* stream, Component& component) {
        stream->readString(component.name);
    }

    TransformComponent& Component::getTransform() {
        return gameObject->getTransform();
    }

    GameObject* Component::getGameObject() {
        return gameObject;
    }
}
