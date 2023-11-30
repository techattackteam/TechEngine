#include "Collider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    Collider::Collider(GameObject *gameObject, PhysicsEngine &physicsEngine, std::string name) : physicsEngine(physicsEngine), Component(gameObject, name) {
    }

    Collider::~Collider() {
        physicsEngine.removeCollider(gameObject->getTag(), this);
    }

    void Collider::setOffset(glm::vec3 offset) {
        this->offset = offset;
    }

    glm::vec3 Collider::getOffset() {
        return offset;
    }

    bool Collider::hasRigidBody() {
        return gameObject->hasComponent<RigidBody>();
    }


}