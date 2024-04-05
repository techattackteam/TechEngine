#include "SphereCollider.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    SphereCollider::SphereCollider(GameObject* gameObject) : Collider(gameObject, "SphereCollider") {
        offset = glm::vec3(0, 0, 0);
        radius = 0.5f;
    }

    SphereCollider::SphereCollider(GameObject* gameObject, glm::vec3 offset, float radius) : Collider(gameObject, "SphereCollider"), radius(radius) {
        this->offset = offset;
    }

    float SphereCollider::getRadius() {
        return radius;
    }

    void SphereCollider::setRadius(float radius) {
        this->radius = radius;
        EventDispatcher::getInstance().dispatch(new AddColliderEvent(this));
    }
}
