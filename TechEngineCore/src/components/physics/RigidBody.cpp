#include "RigidBody.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/registry/AddRigidBodyEvent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    RigidBody::RigidBody(GameObject* gameObject, SystemsRegistry& systemsRegistry) : Component(gameObject, systemsRegistry, "RigidBody") {
        systemsRegistry.getSystem<EventDispatcher>().dispatch(new AddRigidBodyEvent(this));
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

    Component* RigidBody::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        RigidBody* rigidBody = (RigidBody*)componentToCopy;
        auto* component = new RigidBody(gameObjectToAttach, systemsRegistry);
        component->setMass(rigidBody->mass);
        component->setDensity(rigidBody->density);
        return component;
    }
}
