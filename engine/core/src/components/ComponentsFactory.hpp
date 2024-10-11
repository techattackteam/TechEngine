#pragma once
#include "Components.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    class ComponentsFactory {
    public:
        static Tag createTag(const std::string& name, const std::string& uuid) {
            return Tag(name, uuid);
        }

        static Transform createTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
            return Transform();
        }

        static BoxCollider createBoxCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 offset, glm::vec3 size) {
            const JPH::BodyID& bodyID = physicsEngine.createBody(ColliderType::BOX, transform, offset, size);
            return BoxCollider(bodyID);
        }

        static SphereCollider createSphereCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 offset, float radius) {
            const JPH::BodyID& bodyID = physicsEngine.createBody(ColliderType::SPHERE, transform, offset, glm::vec3(radius));
            return SphereCollider(bodyID);
        }
    };
}
