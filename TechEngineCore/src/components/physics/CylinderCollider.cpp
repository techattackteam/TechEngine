#include "CylinderCollider.hpp"

#include "event/EventDispatcher.hpp"
#include "event/events/physics/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    CylinderCollider::CylinderCollider(GameObject* gameObject) : Collider(gameObject, "CylinderCollider") {
        this->gameObject = gameObject;
        this->radius = 0.5f;
        this->height = 1.0f;
    }

    CylinderCollider::CylinderCollider(GameObject* gameObject, glm::vec3 offset, float radius, float height) : Collider(gameObject, "CylinderCollider"), radius(radius), height(height) {
        this->offset = offset;
    }


    float CylinderCollider::getRadius() const {
        return radius;
    }

    void CylinderCollider::setRadius(float radius) {
        this->radius = radius;
        EventDispatcher::getInstance().dispatch(new AddColliderEvent(this));
    }

    float CylinderCollider::getHeight() const {
        return height;
    }

    void CylinderCollider::setHeight(float height) {
        this->height = height;
        EventDispatcher::getInstance().dispatch(new AddColliderEvent(this));
    }
}
