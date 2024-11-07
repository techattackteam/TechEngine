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

        static StaticBody createStaticBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::BodyID& bodyID = physicsEngine.createStaticBody(tag, transform);
            return StaticBody(bodyID);
        }

        static KinematicBody createKinematicBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::BodyID& bodyID = physicsEngine.createKinematicBody(tag, transform);
            return KinematicBody(bodyID);
        }

        static RigidBody createRigidBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::BodyID& bodyID = physicsEngine.createRigidBody(tag, transform);
            return RigidBody(bodyID);
        }

        static BoxCollider createBoxCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 scale) {
            physicsEngine.createBoxCollider(tag, transform, center, scale);
            return BoxCollider(center, scale);
        }

        static SphereCollider createSphereCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 center, float radius) {
            physicsEngine.createSphereCollider(tag, transform, center, radius);
            return SphereCollider();
        }

        static CapsuleCollider createCapsuleCollider(PhysicsEngine& physicsEngine, Tag tag, const Transform& transform, glm::vec3 center, float height, float radius) {
            physicsEngine.createCapsuleCollider(tag, transform, center, height, radius);
            return CapsuleCollider();
        }

        static CylinderCollider createCylinderCollider(PhysicsEngine& physicsEngine, Tag tag, const Transform& transform, glm::vec3 center, float height, float radius) {
            physicsEngine.createCylinderCollider(tag, transform, center, height, radius);
            return CylinderCollider();
        }

        static BoxTrigger createBoxTrigger(PhysicsEngine& physicsEngine, Tag tag, const Transform& transform, glm::vec3 center, glm::vec3 scale) {
            const JPH::BodyID& bodyID = physicsEngine.createBoxTrigger(tag, transform, center, scale);
            return BoxTrigger(bodyID);
        }

        static void DestroyBoxCollider(PhysicsEngine& physicsEngine, const Tag& tag) {
            physicsEngine.removeCollider(tag);
        }

        static void DestroyBody(PhysicsEngine& physicsEngine, const Tag& tag) {
            physicsEngine.removeBody(tag);
        }
    };
}
