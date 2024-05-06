#include "NetworkSync.hpp"

#include "serialization/BufferStream.hpp"

namespace TechEngine {
    NetworkSync::NetworkSync(GameObject* gameObject, EventDispatcher& eventDispatcher) : Component(gameObject, eventDispatcher, "NetworkSync") {
    }

    NetworkSync::~NetworkSync() {
    }

    void NetworkSync::update() {
    }

    void NetworkSync::fixedUpdate() {
    }

    Component* NetworkSync::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        return nullptr;
    }

    void NetworkSync::Serialize(StreamWriter* stream) {
        Component::Serialize(stream);
    }

    void NetworkSync::Deserialize(StreamReader* stream) {
        Component::Deserialize(stream);
    }
}
