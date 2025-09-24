#pragma once
#include "Components.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    class ComponentsFactory {
    public:
        static Tag createTag(const std::string& name, const std::string& uuid) {
            Tag tag = Tag();
            tag.setName(name);
            char* newUuid = new char[uuid.size() + 1];
            strcpy_s(newUuid, uuid.size() + 1, uuid.c_str());
            tag.uuid = newUuid;
            return tag;
        }

        static Transform createTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
            return Transform();
        }
#pragma region Mesh
        static MeshRenderer createMeshRenderer(Mesh* mesh, Material* material) {
            MeshRenderer meshRenderer;
            meshRenderer.mesh = mesh;
            meshRenderer.material = material;
            return meshRenderer;
        }
#pragma endregion

#pragma region Physics Components
        static StaticBody createStaticBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::uint32 index = physicsEngine.createStaticBody(tag, transform);
            return StaticBody(index);
        }

        static KinematicBody createKinematicBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::uint32 index = physicsEngine.createKinematicBody(tag, transform);
            return KinematicBody(index);
        }

        static RigidBody createRigidBody(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
            const JPH::uint32 index = physicsEngine.createRigidBody(tag, transform);
            return RigidBody(index);
        }

        static BoxCollider createBoxCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 scale) {
            physicsEngine.createCollider(Shape::Cube, tag, transform, center, scale);
            return BoxCollider(center, scale);
        }

        static SphereCollider createSphereCollider(PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform, glm::vec3 center, float radius) {
            physicsEngine.createCollider(Shape::Sphere, tag, transform, center, glm::vec3(radius));
            return SphereCollider();
        }

        static CapsuleCollider createCapsuleCollider(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const float height, const float radius) {
            physicsEngine.createCollider(Shape::Capsule, tag, transform, center, glm::vec3(radius, height, radius));
            return CapsuleCollider();
        }

        static CylinderCollider createCylinderCollider(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const float height, const float radius) {
            physicsEngine.createCollider(Shape::Cylinder, tag, transform, center, glm::vec3(radius, height, radius));
            return CylinderCollider();
        }

        static BoxTrigger createBoxTrigger(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const glm::vec3 scale) {
            const JPH::uint32 index = physicsEngine.createTrigger(Shape::Cube, tag, transform, center, scale);
            return BoxTrigger(index);
        }

        static SphereTrigger createSphereTrigger(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const float radius) {
            const JPH::uint32 index = physicsEngine.createTrigger(Shape::Sphere, tag, transform, center, glm::vec3(radius));
            return SphereTrigger(index);
        }

        static CapsuleTrigger createCapsuleTrigger(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const float height, const float radius) {
            const JPH::uint32 index = physicsEngine.createTrigger(Shape::Capsule, tag, transform, center, glm::vec3(radius, height, radius));
            return CapsuleTrigger(index);
        }

        static CylinderTrigger createCylinderTrigger(PhysicsEngine& physicsEngine, const Tag tag, const Transform& transform, const glm::vec3 center, const float height, const float radius) {
            const JPH::uint32 index = physicsEngine.createTrigger(Shape::Cylinder, tag, transform, center, glm::vec3(radius, height, radius));
            return CylinderTrigger(index);
        }

        static void DestroyBoxCollider(PhysicsEngine& physicsEngine, const Tag& tag) {
            physicsEngine.removeCollider(tag);
        }

        static void DestroyBody(PhysicsEngine& physicsEngine, const Tag& tag) {
            physicsEngine.removeBody(tag);
        }
#pragma endregion

#pragma region Audio Components

        static AudioListener createAudioListener() {
            return AudioListener();
        }

        static AudioEmitter createAudioEmitter(float volume = 1.0f, float pitch = 1.0f, bool loop = false) {
            AudioEmitter emitter;
            emitter.volume = volume;
            emitter.pitch = pitch;
            emitter.loop = loop;
            return emitter;
        }
#pragma endregion
    };
}
