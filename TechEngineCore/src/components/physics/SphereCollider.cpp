#include "SphereCollider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    SphereCollider::SphereCollider(GameObject *gameObject, PhysicsEngine &physicsEngine) : Collider(gameObject, physicsEngine, "SphereCollider") {
        offset = glm::vec3(0, 0, 0);
        radius = 0.5f;
    }

    SphereCollider::SphereCollider(GameObject *gameObject, PhysicsEngine &physicsEngine, glm::vec3 offset, float radius) : Collider(gameObject, physicsEngine, "SphereCollider"), radius(radius) {
        this->offset = offset;
    }

    float SphereCollider::getRadius() {
        return radius;
    }

    void SphereCollider::setRadius(float radius) {
        this->radius = radius;
        physicsEngine.addCollider(this);
    }

}