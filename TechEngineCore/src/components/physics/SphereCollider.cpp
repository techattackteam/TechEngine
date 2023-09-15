#include "SphereCollider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    SphereCollider::SphereCollider(GameObject *gameObject) : Collider(gameObject, "SphereCollider") {
        offset = glm::vec3(0, 0, 0);
        radius = 1;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    SphereCollider::SphereCollider(GameObject *gameObject, glm::vec3 offset, float radius) :
            Collider(gameObject, "SphereCollider"), offset(offset), radius(radius) {
        PhysicsEngine::getInstance()->addCollider(this);
    }

    glm::vec3 SphereCollider::getOffset() {
        return offset;
    }

    float SphereCollider::getRadius() {
        return radius;
    }

    void SphereCollider::setOffset(glm::vec3 offset) {
        this->offset = offset;
    }

    void SphereCollider::setRadius(float radius) {
        this->radius = radius;
    }
}