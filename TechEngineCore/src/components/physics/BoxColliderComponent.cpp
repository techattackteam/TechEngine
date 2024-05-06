#include "BoxColliderComponent.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, EventDispatcher& eventDispatcher) : Collider(gameObject, eventDispatcher, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        eventDispatcher.dispatch(new AddColliderEvent(this));
    }

    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, EventDispatcher& eventDispatcher, glm::vec3 size, glm::vec3 offset) : Collider(gameObject, eventDispatcher, "BoxColliderComponent"), size(size) {
        this->offset = offset;
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
        eventDispatcher.dispatch(new AddColliderEvent(this));
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }

    Component* BoxColliderComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        auto* component = dynamic_cast<BoxColliderComponent*>(componentToCopy);
        return new BoxColliderComponent(gameObjectToAttach, eventDispatcher, component->size, component->offset);
    }
}
