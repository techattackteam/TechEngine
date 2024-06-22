#include "BoxColliderComponent.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry) : Collider(gameObject, systemsRegistry, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddColliderEvent(this));
    }

    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry, glm::vec3 size, glm::vec3 offset) : Collider(gameObject, systemsRegistry, "BoxColliderComponent"), size(size) {
        this->offset = offset;
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddColliderEvent(this));
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }

    Component* BoxColliderComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        auto* component = dynamic_cast<BoxColliderComponent*>(componentToCopy);
        return new BoxColliderComponent(gameObjectToAttach, systemsRegistry, component->size, component->offset);
    }
}
