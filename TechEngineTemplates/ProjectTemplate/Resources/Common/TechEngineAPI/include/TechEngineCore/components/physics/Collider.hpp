#pragma once

#include "components/Component.hpp"
#include <glm/glm.hpp>

namespace TechEngine {
    class PhysicsEngine;

    class Collider : public Component {
    protected:
        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);

    public:
        Collider(GameObject*, EventDispatcher& eventDispatcher, std::string name);

        ~Collider() override;

        void setOffset(glm::vec3 offset);

        glm::vec3 getOffset();

        bool hasRigidBody();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) = 0;
    };
}
