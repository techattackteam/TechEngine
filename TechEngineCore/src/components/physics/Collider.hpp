#pragma once

#include "components/Component.hpp"

namespace TechEngine {
    class PhysicsEngine;

    class Collider : public Component {
    protected:
        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);
        PhysicsEngine &physicsEngine;
    public:
        Collider(GameObject *gameObject, PhysicsEngine &physicsEngine, std::string name);

        ~Collider() override;

        void setOffset(glm::vec3 offset);

        glm::vec3 getOffset();

        bool hasRigidBody();

    };
}
