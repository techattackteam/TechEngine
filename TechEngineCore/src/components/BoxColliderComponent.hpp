#pragma once

#include "Component.hpp"

namespace TechEngine {
    class BoxColliderComponent : public Component {
    private:
        glm::vec3 size;
        glm::vec3 offset;

        bool dynamic = true;
    public:
        BoxColliderComponent(GameObject *gameObject);

        BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset, bool dynamic = true);

        void setSize(glm::vec3 size);

        glm::vec3 getSize();

        void setOffset(glm::vec3 offset);

        glm::vec3 getOffset();

        void setDynamic(bool dynamic);

        bool isDynamic();
    };
}
