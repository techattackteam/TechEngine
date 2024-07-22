#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class CORE_DLL SphereCollider : public Collider {
    private:
        float radius;

    public:
        SphereCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry);

        SphereCollider(GameObject* gameObject, SystemsRegistry& systemsRegistry, glm::vec3 offset, float radius);

        float getRadius();

        void setRadius(float radius);

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
