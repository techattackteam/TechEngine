#pragma once

#include "glm/vec3.hpp"
#include "Collider.hpp"

namespace TechEngine {
    class SphereCollider : public Collider {
    private:
        glm::vec3 offset;
        float radius;

    public:
        SphereCollider(GameObject *gameObject);

        SphereCollider(GameObject *gameObject, glm::vec3 offset, float radius);

        glm::vec3 getOffset();

        void setOffset(glm::vec3 offset);

        float getRadius();

        void setRadius(float radius);

    };
}
