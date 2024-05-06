#include "Component.hpp"

#include "TransformComponent.hpp"
#include "scene/GameObject.hpp"

#include <utility>

namespace TechEngine {
    Component::Component(GameObject* gameObject,
                         EventDispatcher& eventDispatcher,
                         std::string name) : gameObject(gameObject),
                                             eventDispatcher(eventDispatcher),
                                             name(std::move(name)) {
    }

    Component::~Component() {
    }

    void Component::fixedUpdate() {
    }

    void Component::update() {
    }

    void Component::Serialize(StreamWriter* stream) {
        stream->writeString(name);
    }

    void Component::Deserialize(StreamReader* stream) {
        stream->readString(name);
    }

    TransformComponent& Component::getTransform() {
        return gameObject->getTransform();
    }

    GameObject* Component::getGameObject() {
        return gameObject;
    }
}
