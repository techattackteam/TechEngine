#pragma once

#include "glm/vec3.hpp"
#include "Collider.hpp"

namespace TechEngine {
    class SphereCollider : public Collider {
    private:
        float radius;

    public:
        SphereCollider(GameObject *gameObject);

        SphereCollider(GameObject *gameObject, glm::vec3 offset, float radius);

        ~SphereCollider() override;

        float getRadius();

        void setRadius(float radius);

    };
}
