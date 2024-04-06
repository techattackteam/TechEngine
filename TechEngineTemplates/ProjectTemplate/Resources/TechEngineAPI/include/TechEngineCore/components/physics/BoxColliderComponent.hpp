#pragma once

#include "Collider.hpp"

namespace TechEngine {
    class BoxColliderComponent : public Collider {
    private:
        glm::vec3 size;

    public:
        BoxColliderComponent(GameObject* gameObject);

        BoxColliderComponent(GameObject* gameObject, glm::vec3 size, glm::vec3 offset);

        void setSize(glm::vec3 size);

        glm::vec3 getSize();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
