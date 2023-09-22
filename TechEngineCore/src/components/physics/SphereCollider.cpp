#include "SphereCollider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    SphereCollider::SphereCollider(GameObject *gameObject) : Collider(gameObject, "SphereCollider") {
        offset = glm::vec3(0, 0, 0);
        radius = 0.5f;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    SphereCollider::SphereCollider(GameObject *gameObject, glm::vec3 offset, float radius) :
            Collider(gameObject, "SphereCollider"), radius(radius) {
        this->offset = offset;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    SphereCollider::~SphereCollider() {
        PhysicsEngine::getInstance()->removeCollider(gameObject->getTag(), this);
    }

    float SphereCollider::getRadius() {
        return radius;
    }

    void SphereCollider::setRadius(float radius) {
        this->radius = radius;
        PhysicsEngine::getInstance()->addCollider(this);
    }

}