#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class CORE_DLL BoxColliderComponent : public Collider {
    private:
        glm::vec3 size;

    public:
        BoxColliderComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry);

        BoxColliderComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry, glm::vec3 size, glm::vec3 offset);

        void setSize(glm::vec3 size);

        glm::vec3 getSize();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
