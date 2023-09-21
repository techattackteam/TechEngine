#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class CylinderCollider : public Collider {
    private:
        float radius;
        float height;
    public:
        CylinderCollider(GameObject *gameObject);

        CylinderCollider(GameObject *gameObject, glm::vec3 offset, float radius, float height);

        ~CylinderCollider() override;

        float getRadius() const;

        void setRadius(float radius);

        float getHeight() const;

        void setHeight(float height);
    };
}
