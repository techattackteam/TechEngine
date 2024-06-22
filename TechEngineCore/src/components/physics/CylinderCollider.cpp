#include "CylinderCollider.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    CylinderCollider::CylinderCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry) : Collider(gameObject, systemsRegistry, "CylinderCollider") {
        this->gameObject = gameObject;
        this->radius = 0.5f;
        this->height = 1.0f;
    }

    CylinderCollider::CylinderCollider(GameObject* gameObject,
                                       SystemsRegistry& systemsRegistry,
                                       glm::vec3 offset,
                                       float radius,
                                       float height) : Collider(gameObject, systemsRegistry, "CylinderCollider"),
                                                       radius(radius),
                                                       height(height) {
        this->offset = offset;
    }


    float CylinderCollider::getRadius() const {
        return radius;
    }

    void CylinderCollider::setRadius(float radius) {
        this->radius = radius;
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddColliderEvent(this));
    }

    float CylinderCollider::getHeight() const {
        return height;
    }

    void CylinderCollider::setHeight(float height) {
        this->height = height;
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddColliderEvent(this));
    }

    Component* CylinderCollider::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        CylinderCollider* collider = (CylinderCollider*)componentToCopy;
        auto* component = new CylinderCollider(gameObjectToAttach, systemsRegistry, collider->offset, collider->radius, collider->height);
        return component;
    }
}
