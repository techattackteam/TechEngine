#include "SceneSerializer.hpp"

#include "Scene.hpp"
#include "components/ComponentsFactory.hpp"
#include "core/UUID.hpp"
#include "resources/ResourcesManager.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    SceneSerializer::SceneSerializer(Scene& scene
                                     , ResourcesManager& resourcesManager
                                     , PhysicsEngine& physicsEngine) : m_scene(scene),
                                                                       m_resourcesManager(resourcesManager),
                                                                       m_physicsEngine(physicsEngine) {
    }

    void SceneSerializer::serialize(std::ofstream& stream) const {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << m_scene.getName();
        out << YAML::Key << "Archetypes" << YAML::Value << YAML::BeginSeq;
        for (Archetype& archetype: m_scene.m_archetypesManager.m_archetypes) {
            out << YAML::BeginMap;
            out << YAML::Key << "ID" << YAML::Value << archetype.m_id;
            out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
            for (Entity& entity: archetype.m_entities) {
                out << YAML::BeginMap;
                out << YAML::Key << "ID" << YAML::Value << entity;
                out << YAML::Key << "Components" << YAML::Value << YAML::BeginMap;
                for (const auto& pair: archetype.m_componentData) {
                    serializeComponent(archetype, entity, pair.first, out);
                }
                out << YAML::EndMap;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        stream << out.c_str();
    }

    std::string SceneSerializer::getSceneName(const std::filesystem::path& path) {
        YAML::Node node = YAML::LoadFile(path.string());
        return node["Name"].as<std::string>();
    }

    void SceneSerializer::deserialize(const std::filesystem::path& path) const {
        YAML::Node node = YAML::LoadFile(path.string());
        m_scene.setName(node["Name"].as<std::string>());
        for (const YAML::Node& archetypeNode: node["Archetypes"]) {
            for (const YAML::Node& entityNode: archetypeNode["Entities"]) {
                Entity entity = entityNode["ID"].as<Entity>();
                std::vector<ComponentTypeID> componentTypes;
                std::unordered_map<ComponentTypeID, void*> components;
                const YAML::Node& componentNode = entityNode["Components"];
                m_scene.m_archetypesManager.registerEntity(entity);
                deserializeComponentNode(entity, componentNode);
            }
        }
    }

    void SceneSerializer::serializeComponent(Archetype& archetype, const Entity& entity, const ComponentTypeID& typeID, YAML::Emitter& out) const {
        if (typeID == ComponentType::get<Transform>()) {
            const Transform& transform = archetype.getComponent<Transform>(entity);
            Transform::serialize(transform, out);
        } else if (typeID == ComponentType::get<Tag>()) {
            const Tag& tag = archetype.getComponent<Tag>(entity);
            Tag::serialize(tag, out);
        } else if (typeID == ComponentType::get<Camera>()) {
            const Camera& camera = archetype.getComponent<Camera>(entity);
            Camera::serialize(camera, out);
        } else if (typeID == ComponentType::get<MeshRenderer>()) {
            const MeshRenderer& meshRenderer = archetype.getComponent<MeshRenderer>(entity);
            MeshRenderer::serialize(meshRenderer, out);
        } else if (typeID == ComponentType::get<StaticBody>()) {
            const StaticBody& staticBody = archetype.getComponent<StaticBody>(entity);
            StaticBody::serialize(staticBody, out);
        } else if (typeID == ComponentType::get<KinematicBody>()) {
            const KinematicBody& kinematicBody = archetype.getComponent<KinematicBody>(entity);
            KinematicBody::serialize(kinematicBody, out);
        } else if (typeID == ComponentType::get<RigidBody>()) {
            const RigidBody& rigidBody = archetype.getComponent<RigidBody>(entity);
            RigidBody::serialize(rigidBody, out);
        } else if (typeID == ComponentType::get<BoxCollider>()) {
            const BoxCollider& boxCollider = archetype.getComponent<BoxCollider>(entity);
            BoxCollider::serialize(boxCollider, out);
        } else if (typeID == ComponentType::get<SphereCollider>()) {
            const SphereCollider& sphereCollider = archetype.getComponent<SphereCollider>(entity);
            SphereCollider::serialize(sphereCollider, out);
        } else if (typeID == ComponentType::get<CapsuleCollider>()) {
            const CapsuleCollider& capsuleCollider = archetype.getComponent<CapsuleCollider>(entity);
            CapsuleCollider::serialize(capsuleCollider, out);
        } else if (typeID == ComponentType::get<CylinderCollider>()) {
            const CylinderCollider& cylinderCollider = archetype.getComponent<CylinderCollider>(entity);
            CylinderCollider::serialize(cylinderCollider, out);
        } else if (typeID == ComponentType::get<BoxTrigger>()) {
            const BoxTrigger& boxTrigger = archetype.getComponent<BoxTrigger>(entity);
            BoxTrigger::serialize(boxTrigger, out);
        }
    }

    void SceneSerializer::deserializeComponentNode(const Entity& entity, const YAML::Node& componentNode) const {
        if (componentNode["Tag"]) {
            Tag tag = Tag::deserialize(componentNode["Tag"]);
            UUID::registerUUID(std::stoull(tag.getUuid()));
            m_scene.m_archetypesManager.addComponent(entity, tag);
        }
        if (componentNode["Transform"]) {
            Transform transform = Transform::deserialize(componentNode["Transform"]);
            m_scene.m_archetypesManager.addComponent(entity, transform);
        }
        if (componentNode["Camera"]) {
            Camera camera = Camera::deserialize(componentNode["Camera"]);
            m_scene.m_archetypesManager.addComponent(entity, camera);
        }
        if (componentNode["MeshRenderer"]) {
            MeshRenderer meshRenderer = MeshRenderer::deserialize(componentNode, m_resourcesManager);
            m_scene.m_archetypesManager.addComponent(entity, meshRenderer);
        }

        if (componentNode["StaticBody"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            StaticBody staticBody = StaticBody::deserialize(componentNode["StaticBody"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, staticBody);
        }

        if (componentNode["KinematicBody"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            KinematicBody kinematicBody = KinematicBody::deserialize(componentNode["KinematicBody"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, kinematicBody);
        }

        if (componentNode["RigidBody"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            RigidBody rigidBody = RigidBody::deserialize(componentNode["RigidBody"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, rigidBody);
        }


        if (componentNode["BoxCollider"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            BoxCollider boxCollider = BoxCollider::deserialize(componentNode["BoxCollider"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, boxCollider);
        }

        if (componentNode["SphereCollider"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            SphereCollider sphereCollider = SphereCollider::deserialize(componentNode["SphereCollider"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, sphereCollider);
        }

        if (componentNode["CapsuleCollider"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            CapsuleCollider capsuleCollider = CapsuleCollider::deserialize(componentNode["CapsuleCollider"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, capsuleCollider);
        }

        if (componentNode["CylinderCollider"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            CylinderCollider cylinderCollider = CylinderCollider::deserialize(componentNode["CylinderCollider"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, cylinderCollider);
        }

        if (componentNode["BoxTrigger"]) {
            const Tag& tag = m_scene.m_archetypesManager.getComponent<Tag>(entity);
            const Transform& transform = m_scene.m_archetypesManager.getComponent<Transform>(entity);
            BoxTrigger boxTrigger = BoxTrigger::deserialize(componentNode["BoxTrigger"], m_physicsEngine, tag, transform);
            m_scene.m_archetypesManager.addComponent(entity, boxTrigger);
        }
    }

    void Tag::serialize(const Tag& tag, YAML::Emitter& out) {
        out << YAML::Key << "Tag" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << tag.getName();
        out << YAML::Key << "UUID" << YAML::Value << tag.getUuid();
        out << YAML::EndMap;
    }

    Tag Tag::deserialize(const YAML::Node& node) {
        return ComponentsFactory::createTag(node["Name"].as<std::string>(), node["UUID"].as<std::string>());
    }

    void Transform::serialize(const Transform& transform, YAML::Emitter& out) {
        out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.m_position.x << transform.m_position.y << transform.m_position.z << YAML::EndSeq;
        out << YAML::Key << "Rotation" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.m_rotation.x << transform.m_rotation.y << transform.m_rotation.z << YAML::EndSeq;
        out << YAML::Key << "Scale" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.m_scale.x << transform.m_scale.y << transform.m_scale.z << YAML::EndSeq;
        out << YAML::EndMap;
    }

    Transform Transform::deserialize(const YAML::Node& node) {
        Transform transform = ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
        transform.m_position = glm::vec3(node["Position"].as<glm::vec3>());
        transform.m_rotation = glm::vec3(node["Rotation"].as<glm::vec3>());
        transform.m_scale = glm::vec3(node["Scale"].as<glm::vec3>());
        return transform;
    }

    void Camera::serialize(const Camera& camera, YAML::Emitter& out) {
        out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MainCamera" << YAML::Value << camera.mainCamera;
        out << YAML::Key << "ViewMatrix" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out << camera.viewMatrix[i][j];
            }
        }
        out << YAML::EndSeq;
        out << YAML::Key << "ProjectionMatrix" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out << camera.projectionMatrix[i][j];
            }
        }
        out << YAML::EndSeq;
        out << YAML::Key << "FOV" << YAML::Value << camera.fov.x;
        out << YAML::Key << "NearPlane" << YAML::Value << camera.nearPlane;
        out << YAML::Key << "FarPlane" << YAML::Value << camera.farPlane;
        out << YAML::Key << "OrthoSize" << YAML::Value << camera.orthoSize;
        out << YAML::Key << "AspectRatio" << YAML::Value << camera.aspectRatio;
        out << YAML::EndMap;
    }

    Camera Camera::deserialize(const YAML::Node& node) {
        Camera camera;
        camera.mainCamera = node["MainCamera"].as<bool>();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                camera.viewMatrix[i][j] = node["ViewMatrix"][i * 4 + j].as<float>();
                camera.projectionMatrix[i][j] = node["ProjectionMatrix"][i * 4 + j].as<float>();
            }
        }
        camera.fov = glm::vec1(node["FOV"].as<float>());
        camera.nearPlane = node["NearPlane"].as<float>();
        camera.farPlane = node["FarPlane"].as<float>();
        camera.orthoSize = node["OrthoSize"].as<float>();
        camera.aspectRatio = node["AspectRatio"].as<float>();
        return camera;
    }

    void MeshRenderer::serialize(const MeshRenderer& meshRenderer, YAML::Emitter& out) {
        out << YAML::Key << "MeshRenderer" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Mesh" << YAML::Value << meshRenderer.mesh.getName();
        out << YAML::Key << "Material" << YAML::Value << meshRenderer.material.getName();
        out << YAML::EndMap;
    }

    MeshRenderer MeshRenderer::deserialize(const YAML::Node& node, ResourcesManager& resourcesManager) {
        std::string meshName = node["MeshRenderer"]["Mesh"].as<std::string>();
        std::string materialName = node["MeshRenderer"]["Material"].as<std::string>();
        Mesh& mesh = resourcesManager.getMesh(meshName);
        Material& material = resourcesManager.getMaterial(materialName);
        MeshRenderer meshRenderer(mesh, material);
        //meshRenderer.paintMesh();
        return meshRenderer;
    }
#pragma region Physics Components
    void BoxCollider::serialize(const BoxCollider& boxCollider, YAML::Emitter& out) {
        out << YAML::Key << "BoxCollider" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Center" << YAML::Value << YAML::Flow << YAML::BeginSeq << boxCollider.center.x << boxCollider.center.y << boxCollider.center.z << YAML::EndSeq;
        out << YAML::Key << "Size" << YAML::Value << YAML::Flow << YAML::BeginSeq << boxCollider.size.x << boxCollider.size.y << boxCollider.size.z << YAML::EndSeq;
        out << YAML::EndMap;
    }

    BoxCollider BoxCollider::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center = glm::vec3(node["Center"].as<glm::vec3>());
        glm::vec3 size = glm::vec3(node["Size"].as<glm::vec3>());
        return ComponentsFactory::createBoxCollider(m_physicsEngine, tag, transform, center, size);
    }

    void SphereCollider::serialize(const SphereCollider& sphereCollider, YAML::Emitter& out) {
        out << YAML::Key << "SphereCollider" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Center" << YAML::Value << YAML::Flow << YAML::BeginSeq << sphereCollider.center.x << sphereCollider.center.y << sphereCollider.center.z << YAML::EndSeq;
        out << YAML::Key << "Radius" << YAML::Value << sphereCollider.radius;
        out << YAML::EndMap;
    }

    SphereCollider SphereCollider::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 center = glm::vec3(node["Center"].as<glm::vec3>());
        float radius = node["Radius"].as<float>();

        return ComponentsFactory::createSphereCollider(m_physicsEngine, tag, transform, center, radius);
    }

    void CapsuleCollider::serialize(const CapsuleCollider& capsuleCollider, YAML::Emitter& out) {
        out << YAML::Key << "CapsuleCollider" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Center" << YAML::Value << YAML::Flow << YAML::BeginSeq << capsuleCollider.center.x << capsuleCollider.center.y << capsuleCollider.center.z << YAML::EndSeq;
        out << YAML::Key << "Height" << YAML::Value << capsuleCollider.height;
        out << YAML::Key << "Radius" << YAML::Value << capsuleCollider.radius;
        out << YAML::EndMap;
    }

    CapsuleCollider CapsuleCollider::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        const glm::vec3 center = glm::vec3(node["Center"].as<glm::vec3>());
        const float height = node["Height"].as<float>();
        const float radius = node["Radius"].as<float>();
        return ComponentsFactory::createCapsuleCollider(m_physicsEngine, tag, transform, center, height, radius);
    }

    void CylinderCollider::serialize(const CylinderCollider& cylinderCollider, YAML::Emitter& out) {
        out << YAML::Key << "CylinderCollider" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Center" << YAML::Value << YAML::Flow << YAML::BeginSeq << cylinderCollider.center.x << cylinderCollider.center.y << cylinderCollider.center.z << YAML::EndSeq;
        out << YAML::Key << "Height" << YAML::Value << cylinderCollider.height;
        out << YAML::Key << "Radius" << YAML::Value << cylinderCollider.radius;
        out << YAML::EndMap;
    }

    CylinderCollider CylinderCollider::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        const glm::vec3 center = glm::vec3(node["Center"].as<glm::vec3>());
        const float height = node["Height"].as<float>();
        const float radius = node["Radius"].as<float>();

        return ComponentsFactory::createCylinderCollider(m_physicsEngine, tag, transform, center, height, radius);
    }


    void StaticBody::serialize(const StaticBody& staticBody, YAML::Emitter& out) {
        out << YAML::Key << "StaticBody" << YAML::Value << YAML::BeginMap;
        out << YAML::EndMap;
    }

    StaticBody StaticBody::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createStaticBody(m_physicsEngine, tag, transform);
    }

    void KinematicBody::serialize(const KinematicBody& kinematicBody, YAML::Emitter& out) {
        out << YAML::Key << "KinematicBody" << YAML::Value << YAML::BeginMap;
        out << YAML::EndMap;
    }

    KinematicBody KinematicBody::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createKinematicBody(m_physicsEngine, tag, transform);
    }

    void RigidBody::serialize(const RigidBody& rigidBody, YAML::Emitter& out) {
        out << YAML::Key << "RigidBody" << YAML::Value << YAML::BeginMap;
        out << YAML::EndMap;
    }

    RigidBody RigidBody::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        return ComponentsFactory::createRigidBody(m_physicsEngine, tag, transform);
    }

    void BoxTrigger::serialize(const BoxTrigger& boxTrigger, YAML::Emitter& out) {
        out << YAML::Key << "BoxTrigger" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Size" << YAML::Value << YAML::Flow << YAML::BeginSeq << boxTrigger.size.x << boxTrigger.size.y << boxTrigger.size.z << YAML::EndSeq;
        out << YAML::Key << "Center" << YAML::Value << YAML::Flow << YAML::BeginSeq << boxTrigger.center.x << boxTrigger.center.y << boxTrigger.center.z << YAML::EndSeq;
        out << YAML::EndMap;
    }

    BoxTrigger BoxTrigger::deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform) {
        glm::vec3 size = glm::vec3(node["Size"].as<glm::vec3>());
        glm::vec3 center = glm::vec3(node["Center"].as<glm::vec3>());
        return ComponentsFactory::createBoxTrigger(m_physicsEngine, tag, transform, center, size);
    }
#pragma endregion
#pragma region Audio Components
    void AudioListenerComponent::serialize(const AudioListenerComponent& audioListener, YAML::Emitter& out) {
        out << YAML::Key << "AudioListener" << YAML::Value << YAML::BeginMap;
        out << YAML::EndMap;
    }

    AudioListenerComponent AudioListenerComponent::deserialize(const YAML::Node& node) {
        AudioListenerComponent audioListener;
        // Currently, no properties to deserialize for AudioListenerComponent
        return audioListener;
    }

    void AudioEmitterComponent::serialize(const AudioEmitterComponent& emitter, YAML::Emitter& out) {
        out << YAML::Key << "AudioEmitter" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Volume" << YAML::Value << emitter.volume;
        out << YAML::Key << "Pitch" << YAML::Value << emitter.pitch;
        out << YAML::Key << "Loop" << YAML::Value << emitter.loop;
        //out << YAML::Key << "AudioClip" << YAML::Value << emitter.audioClip.getName();
        out << YAML::EndMap;
    }

    AudioEmitterComponent AudioEmitterComponent::deserialize(const YAML::Node& node) {
        AudioEmitterComponent emitter;
        emitter.volume = node["Volume"].as<float>();
        emitter.pitch = node["Pitch"].as<float>();
        emitter.loop = node["Loop"].as<bool>();
        //emitter.audioClip = m_resourcesManager.getAudioClip(node["AudioClip"].as<std::string>());
        return emitter;
    }

#pragma endregion
}
