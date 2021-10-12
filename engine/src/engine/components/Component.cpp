#include "Component.hpp"

#include <utility>

namespace Engine {
    Component::Component(ComponentName name) : name(std::move(name)) {

    }

    Component::~Component() {

    }

    void Component::fixedUpdate() {

    }

    void Component::update() {

    }

}
