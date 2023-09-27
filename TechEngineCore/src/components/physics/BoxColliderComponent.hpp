#pragma once

#include "components/Component.hpp"
#include "Collider.hpp"

namespace TechEngine {
    class BoxColliderComponent : public Collider {
    private:
        glm::vec3 size;
    public:
        BoxColliderComponent(GameObject *gameObject);

        BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset);

        ~BoxColliderComponent() override;

        void setSize(glm::vec3 size);

        glm::vec3 getSize();
    };
}
