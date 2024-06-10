#pragma once

#include "components/Component.hpp"

namespace TechEngine {
    class RigidBody : public Component {
    private:
        float mass = 0.1f;
        float density = 0.1f;

    public:
        RigidBody(GameObject* gameObject, EventDispatcher& eventDispatcher);

        void setMass(float mass);

        float getMass() const;

        void setDensity(float density);

        float getDensity() const;

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
