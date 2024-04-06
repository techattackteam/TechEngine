#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class SphereCollider : public Collider {
    private:
        float radius;

    public:
        SphereCollider(GameObject* gameObject);

        SphereCollider(GameObject* gameObject, glm::vec3 offset, float radius);

        float getRadius();

        void setRadius(float radius);

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
