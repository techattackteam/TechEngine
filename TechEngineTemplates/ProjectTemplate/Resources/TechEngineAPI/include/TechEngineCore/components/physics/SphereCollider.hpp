#pragma once

#include "glm/vec3.hpp"
#include "Collider.hpp"

namespace TechEngine {
    class SphereCollider : public Collider {
    private:
        float radius;

    public:
        SphereCollider(GameObject *gameObject, PhysicsEngine &physicsEngine);

        SphereCollider(GameObject *gameObject, PhysicsEngine &physicsEngine, glm::vec3 offset, float radius);


        float getRadius();

        void setRadius(float radius);

    };
}
