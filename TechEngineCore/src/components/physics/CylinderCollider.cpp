#include "CylinderCollider.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    CylinderCollider::CylinderCollider(GameObject* gameObject, EventDispatcher& eventDispatcher) : Collider(gameObject, eventDispatcher, "CylinderCollider") {
        this->gameObject = gameObject;
        this->radius = 0.5f;
        this->height = 1.0f;
    }

    CylinderCollider::CylinderCollider(GameObject* gameObject, EventDispatcher& eventDispatcher, glm::vec3 offset, float radius, float height) : Collider(gameObject, eventDispatcher, "CylinderCollider"), radius(radius), height(height) {
        this->offset = offset;
    }


    float CylinderCollider::getRadius() const {
        return radius;
    }

    void CylinderCollider::setRadius(float radius) {
        this->radius = radius;
        eventDispatcher.dispatch(new AddColliderEvent(this));
    }

    float CylinderCollider::getHeight() const {
        return height;
    }

    void CylinderCollider::setHeight(float height) {
        this->height = height;
        eventDispatcher.dispatch(new AddColliderEvent(this));
    }

    Component* CylinderCollider::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        CylinderCollider* collider = (CylinderCollider*)componentToCopy;
        auto* component = new CylinderCollider(gameObjectToAttach, eventDispatcher, collider->offset, collider->radius, collider->height);
        return component;
    }
}
