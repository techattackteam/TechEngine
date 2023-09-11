#include "BoxColliderComponent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject) : Component(gameObject, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        offset = glm::vec3(0, 0, 0);
        PhysicsEngine::getInstance()->addBoxCollider(this);
    }


    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset, bool dynamic) :
            Component(gameObject, "BoxColliderComponent"), size(size), offset(offset), dynamic(dynamic) {
        PhysicsEngine::getInstance()->addBoxCollider(this);
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }

    void BoxColliderComponent::setOffset(glm::vec3 offset) {
        this->offset = offset;
    }

    glm::vec3 BoxColliderComponent::getOffset() {
        return offset;
    }

    void BoxColliderComponent::setDynamic(bool dynamic) {
        this->dynamic = dynamic;
        PhysicsEngine::getInstance()->addBoxCollider(this);
    }

    bool BoxColliderComponent::isDynamic() {
        return dynamic;
    }
}
