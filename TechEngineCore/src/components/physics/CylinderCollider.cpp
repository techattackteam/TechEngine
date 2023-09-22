#include "CylinderCollider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {

    CylinderCollider::CylinderCollider(TechEngine::GameObject *gameObject) : Collider(gameObject, "CylinderCollider") {
        this->gameObject = gameObject;
        this->radius = 0.5f;
        this->height = 1.0f;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    CylinderCollider::CylinderCollider(GameObject *gameObject, glm::vec3 offset, float radius, float height) :
            Collider(gameObject, "CylinderCollider"), radius(radius), height(height) {
        this->offset = offset;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    CylinderCollider::~CylinderCollider() {
        PhysicsEngine::getInstance()->removeCollider(gameObject->getTag(), this);
    }

    float CylinderCollider::getRadius() const {
        return radius;
    }

    void CylinderCollider::setRadius(float radius) {
        this->radius = radius;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    float CylinderCollider::getHeight() const {
        return height;
    }

    void CylinderCollider::setHeight(float height) {
        this->height = height;
        PhysicsEngine::getInstance()->addCollider(this);
    }
}
