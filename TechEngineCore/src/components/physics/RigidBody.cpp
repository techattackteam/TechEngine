#include "RigidBody.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    RigidBody::RigidBody(GameObject *gameObject) : Component(gameObject, "RigidBody") {
    }


    void RigidBody::setMass(float mass) {
        this->mass = mass;
    }

    float RigidBody::getMass() const {
        return mass;
    }

    void RigidBody::setDensity(float density) {
        this->density = density;
    }

    float RigidBody::getDensity() const {
        return density;
    }

}