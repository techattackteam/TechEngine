#include "Collider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    Collider::Collider(GameObject *gameObject, std::string name) : Component(gameObject, name) {

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