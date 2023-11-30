#include "CylinderCollider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {

    CylinderCollider::CylinderCollider(GameObject *gameObject, PhysicsEngine &physicsEngine) : Collider(gameObject, physicsEngine, "CylinderCollider") {
        this->gameObject = gameObject;
        this->radius = 0.5f;
        this->height = 1.0f;
    }

    CylinderCollider::CylinderCollider(GameObject *gameObject, PhysicsEngine &physicsEngine, glm::vec3 offset, float radius, float height) :
            Collider(gameObject, physicsEngine, "CylinderCollider"), radius(radius), height(height) {
        this->offset = offset;
    }


    float CylinderCollider::getRadius() const {
        return radius;
    }

    void CylinderCollider::setRadius(float radius) {
        this->radius = radius;
        physicsEngine.addCollider(this);
    }

    float CylinderCollider::getHeight() const {
        return height;
    }

    void CylinderCollider::setHeight(float height) {
        this->height = height;
        physicsEngine.addCollider(this);
    }
}
