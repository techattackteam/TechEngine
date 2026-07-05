#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "serialization/StreamWriter.hpp"
#include "serialization/StreamReader.hpp"

namespace TechEngine {
    void Tag::serialize(StreamWriter* writer, const Tag& tag) {
        writer->writeString(tag.getName());
        writer->writeString(tag.getUuid());
    }

    Tag Tag::deserialize(StreamReader* reader) {
        std::string name, uuid;
        reader->readString(name);
        reader->readString(uuid);
        UUID::registerUUID(std::stoull(uuid));
        return ComponentsFactory::createTag(name, uuid);
    }

    void Hierarchy::serialize(StreamWriter* writer, const Hierarchy& hierarchy) {
        writer->writeRaw(hierarchy.parent);
        writer->writeRaw(hierarchy.firstChild);
        writer->writeRaw(hierarchy.nextSibling);
        writer->writeRaw(hierarchy.previousSibling);
        writer->writeRaw(static_cast<uint32_t>(hierarchy.childrenCount));
    }

    Hierarchy Hierarchy::deserialize(StreamReader* reader) {
        Hierarchy hierarchy;
        reader->readRaw(hierarchy.parent);
        reader->readRaw(hierarchy.firstChild);
        reader->readRaw(hierarchy.nextSibling);
        reader->readRaw(hierarchy.previousSibling);
        uint32_t childrenCount;
        reader->readRaw(childrenCount);
        hierarchy.childrenCount = childrenCount;
        return hierarchy;
    }

    void Transform::serialize(StreamWriter* writer, const Transform& transform) {
        writer->writeRaw(transform.m_position);
        writer->writeRaw(transform.m_rotation);
        writer->writeRaw(transform.m_scale);
    }

    Transform Transform::deserialize(StreamReader* reader) {
        Transform transform;
        reader->readRaw(transform.m_position);
        reader->readRaw(transform.m_rotation);
        reader->readRaw(transform.m_scale);
        transform.calculateUpForwardRight();
        return transform;
    }

    void Camera::serialize(StreamWriter* writer, const Camera& camera) {
        writer->writeRaw(camera.mainCamera);
        writer->writeRaw(camera.viewMatrix);
        writer->writeRaw(camera.projectionMatrix);
        writer->writeRaw(camera.fov);
        writer->writeRaw(camera.nearPlane);
        writer->writeRaw(camera.farPlane);
        writer->writeRaw(camera.orthoSize);
        writer->writeRaw(camera.aspectRatio);
    }

    Camera Camera::deserialize(StreamReader* reader) {
        Camera camera;
        reader->readRaw(camera.mainCamera);
        reader->readRaw(camera.viewMatrix);
        reader->readRaw(camera.projectionMatrix);
        reader->readRaw(camera.fov);
        reader->readRaw(camera.nearPlane);
        reader->readRaw(camera.farPlane);
        reader->readRaw(camera.orthoSize);
        reader->readRaw(camera.aspectRatio);
        return camera;
    }

    void MeshRenderer::serialize(StreamWriter* writer, const MeshRenderer& meshRenderer) {
        writer->writeRaw(meshRenderer.meshUUID);
        writer->writeRaw(meshRenderer.materialUUID);
    }

    MeshRenderer MeshRenderer::deserialize(StreamReader* reader) {
        MeshRenderer meshRenderer;
        reader->readRaw(meshRenderer.meshUUID);
        reader->readRaw(meshRenderer.materialUUID);
        return meshRenderer;
    }

    void PointLight::serialize(StreamWriter* writer, const PointLight& pointLight) {
        writer->writeRaw(pointLight.color);
        writer->writeRaw(pointLight.intensity);
        writer->writeRaw(pointLight.radius);
    }

    PointLight PointLight::deserialize(StreamReader* reader) {
        PointLight pointLight;
        reader->readRaw(pointLight.color);
        reader->readRaw(pointLight.intensity);
        reader->readRaw(pointLight.radius);
        return pointLight;
    }

    void DirectionalLight::serialize(StreamWriter* writer, const DirectionalLight& directionalLight) {
        writer->writeRaw(directionalLight.color);
        writer->writeRaw(directionalLight.intensity);
    }

    DirectionalLight DirectionalLight::deserialize(StreamReader* reader) {
        DirectionalLight directionalLight;
        reader->readRaw(directionalLight.color);
        reader->readRaw(directionalLight.intensity);
        return directionalLight;
    }

    void SpotLight::serialize(StreamWriter* writer, const SpotLight& spotLight) {
        writer->writeRaw(spotLight.color);
        writer->writeRaw(spotLight.intensity);
        writer->writeRaw(spotLight.innerCutoff);
        writer->writeRaw(spotLight.outerCutoff);
    }

    SpotLight SpotLight::deserialize(StreamReader* reader) {
        SpotLight spotLight;
        reader->readRaw(spotLight.color);
        reader->readRaw(spotLight.intensity);
        reader->readRaw(spotLight.innerCutoff);
        reader->readRaw(spotLight.outerCutoff);
        return spotLight;
    }

    void StaticBody::serialize(StreamWriter* writer, const StaticBody& staticBody) {
        // No persistent data beyond existence
    }

    StaticBody StaticBody::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createStaticBody(physicsEngine, tag, transform);
    }

    void KinematicBody::serialize(StreamWriter* writer, const KinematicBody& kinematicBody) {
    }

    KinematicBody KinematicBody::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createKinematicBody(physicsEngine, tag, transform);
    }

    void RigidBody::serialize(StreamWriter* writer, const RigidBody& rigidBody) {
    }

    RigidBody RigidBody::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createRigidBody(physicsEngine, tag, transform);
    }

    void BoxCollider::serialize(StreamWriter* writer, const BoxCollider& boxCollider) {
        writer->writeRaw(boxCollider.center);
        writer->writeRaw(boxCollider.size);
    }

    BoxCollider BoxCollider::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center, size;
        reader->readRaw(center);
        reader->readRaw(size);
        return ComponentsFactory::createBoxCollider(physicsEngine, tag, transform, center, size);
    }

    void SphereCollider::serialize(StreamWriter* writer, const SphereCollider& sphereCollider) {
        writer->writeRaw(sphereCollider.center);
        writer->writeRaw(sphereCollider.radius);
    }

    SphereCollider SphereCollider::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float radius;
        reader->readRaw(center);
        reader->readRaw(radius);
        return ComponentsFactory::createSphereCollider(physicsEngine, tag, transform, center, radius);
    }

    void CapsuleCollider::serialize(StreamWriter* writer, const CapsuleCollider& capsuleCollider) {
        writer->writeRaw(capsuleCollider.center);
        writer->writeRaw(capsuleCollider.height);
        writer->writeRaw(capsuleCollider.radius);
    }

    CapsuleCollider CapsuleCollider::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float height, radius;
        reader->readRaw(center);
        reader->readRaw(height);
        reader->readRaw(radius);
        return ComponentsFactory::createCapsuleCollider(physicsEngine, tag, transform, center, height, radius);
    }

    void CylinderCollider::serialize(StreamWriter* writer, const CylinderCollider& cylinderCollider) {
        writer->writeRaw(cylinderCollider.center);
        writer->writeRaw(cylinderCollider.height);
        writer->writeRaw(cylinderCollider.radius);
    }

    CylinderCollider CylinderCollider::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float height, radius;
        reader->readRaw(center);
        reader->readRaw(height);
        reader->readRaw(radius);
        return ComponentsFactory::createCylinderCollider(physicsEngine, tag, transform, center, height, radius);
    }

    void BoxTrigger::serialize(StreamWriter* writer, const BoxTrigger& boxTrigger) {
        writer->writeRaw(boxTrigger.center);
        writer->writeRaw(boxTrigger.size);
    }

    BoxTrigger BoxTrigger::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center, size;
        reader->readRaw(center);
        reader->readRaw(size);
        return ComponentsFactory::createBoxTrigger(physicsEngine, tag, transform, center, size);
    }

    void SphereTrigger::serialize(StreamWriter* writer, const SphereTrigger& sphereTrigger) {
        writer->writeRaw(sphereTrigger.center);
        writer->writeRaw(sphereTrigger.radius);
    }

    SphereTrigger SphereTrigger::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float radius;
        reader->readRaw(center);
        reader->readRaw(radius);
        return ComponentsFactory::createSphereTrigger(physicsEngine, tag, transform, center, radius);
    }

    void CapsuleTrigger::serialize(StreamWriter* writer, const CapsuleTrigger& capsuleTrigger) {
        writer->writeRaw(capsuleTrigger.center);
        writer->writeRaw(capsuleTrigger.height);
        writer->writeRaw(capsuleTrigger.radius);
    }

    CapsuleTrigger CapsuleTrigger::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float height, radius;
        reader->readRaw(center);
        reader->readRaw(height);
        reader->readRaw(radius);
        return ComponentsFactory::createCapsuleTrigger(physicsEngine, tag, transform, center, height, radius);
    }

    void CylinderTrigger::serialize(StreamWriter* writer, const CylinderTrigger& cylinderTrigger) {
        writer->writeRaw(cylinderTrigger.center);
        writer->writeRaw(cylinderTrigger.height);
        writer->writeRaw(cylinderTrigger.radius);
    }

    CylinderTrigger CylinderTrigger::deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center;
        float height, radius;
        reader->readRaw(center);
        reader->readRaw(height);
        reader->readRaw(radius);
        return ComponentsFactory::createCylinderTrigger(physicsEngine, tag, transform, center, height, radius);
    }

    void AudioListener::serialize(StreamWriter* writer, const AudioListener& audioListener) {
    }

    AudioListener AudioListener::deserialize(StreamReader* reader) {
        return AudioListener();
    }

    void AudioEmitter::serialize(StreamWriter* writer, const AudioEmitter& emitter) {
        writer->writeRaw(emitter.volume);
        writer->writeRaw(emitter.pitch);
        writer->writeRaw(emitter.loop);
    }

    AudioEmitter AudioEmitter::deserialize(StreamReader* reader) {
        AudioEmitter emitter;
        reader->readRaw(emitter.volume);
        reader->readRaw(emitter.pitch);
        reader->readRaw(emitter.loop);
        return emitter;
    }
}
