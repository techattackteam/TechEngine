#include "Component.hpp"

#include <utility>

namespace TechEngine {
    Component::Component(GameObject *gameObject, ComponentName name) : gameObject(gameObject), name(std::move(name)) {

    }

    Component::~Component() {

    }

    void Component::fixedUpdate() {

    }

    void Component::update() {

    }

    Transform &Component::getTransform() {
        return gameObject->getTransform();
    }

}
