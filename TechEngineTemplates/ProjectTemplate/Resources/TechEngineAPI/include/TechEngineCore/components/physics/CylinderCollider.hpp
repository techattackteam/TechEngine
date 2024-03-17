#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class CylinderCollider : public Collider {
    private:
        float radius;
        float height;
    public:
        CylinderCollider(GameObject *gameObject, PhysicsEngine &physicsEngine);

        CylinderCollider(GameObject *gameObject, PhysicsEngine &physicsEngine, glm::vec3 offset, float radius, float height);

        float getRadius() const;

        void setRadius(float radius);

        float getHeight() const;

        void setHeight(float height);
    };
}
