#include "SphereCollider.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    SphereCollider::SphereCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry) : Collider(gameObject, systemsRegistry, "SphereCollider") {
        offset = glm::vec3(0, 0, 0);
        radius = 0.5f;
    }

    SphereCollider::SphereCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry, glm::vec3 offset, float radius) : Collider(gameObject, systemsRegistry, "SphereCollider"), radius(radius) {
        this->offset = offset;
    }

    float SphereCollider::getRadius() {
        return radius;
    }

    void SphereCollider::setRadius(float radius) {
        this->radius = radius;
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddColliderEvent(this));
    }

    Component* SphereCollider::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        SphereCollider* collider = (SphereCollider*)componentToCopy;
        auto* component = new SphereCollider(gameObjectToAttach, systemsRegistry, collider->offset, collider->radius);
        return component;
    }
}
