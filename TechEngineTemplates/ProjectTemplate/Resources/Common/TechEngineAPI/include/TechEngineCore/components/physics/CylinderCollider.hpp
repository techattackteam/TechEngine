#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class CORE_DLL CylinderCollider : public Collider {
    private:
        float radius;
        float height;

    public:
        CylinderCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry);

        CylinderCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry, glm::vec3 offset, float radius, float height);

        float getRadius() const;

        void setRadius(float radius);

        float getHeight() const;

        void setHeight(float height);

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
