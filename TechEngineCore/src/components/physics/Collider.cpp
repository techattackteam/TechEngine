#include "Collider.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    Collider::Collider(GameObject *gameObject, std::string name) : Component(gameObject, name) {

    }

    Collider::Collider(GameObject *gameObject, std::string name, bool dynamic) : Component(gameObject, name), dynamic(dynamic) {

    }

    void Collider::setDynamic(bool dynamic) {
        this->dynamic = dynamic;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    bool Collider::isDynamic() {
        return dynamic;
    }
}