#include "NetworkSync.hpp"

#include "serialization/BufferStream.hpp"

namespace TechEngine {
    NetworkSync::NetworkSync(GameObject* gameObject) : Component(gameObject, "NetworkHandler") {
    }

    NetworkSync::~NetworkSync() {
    }

    void NetworkSync::update() {
        Component::update();
    }

    void NetworkSync::fixedUpdate() {
        Component::fixedUpdate();
    }

    Component* NetworkSync::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        return nullptr;
    }
}
