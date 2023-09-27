#pragma once

#include "components/Component.hpp"

namespace TechEngine {

    class Collider : public Component {
    protected:
        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);

    public:
        Collider(GameObject *gameObject, std::string name);

        void setOffset(glm::vec3 offset);

        glm::vec3 getOffset();

        bool hasRigidBody();

    };
}
