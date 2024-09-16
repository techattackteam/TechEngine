#include "SceneSerializer.hpp"

#include "Scene.hpp"
#include "resources/ResourcesManager.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    SceneSerializer::SceneSerializer(Scene& scene,
                                     ResourcesManager& resourcesManager) : m_scene(scene),
                                                                           m_resourcesManager(resourcesManager) {
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
        }
    }

    void SceneSerializer::deserializeComponentNode(const Entity& entity, const YAML::Node& componentNode) const {
        if (componentNode["Tag"]) {
            Tag tag = Tag::deserialize(componentNode["Tag"]);
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
    }

    void Tag::serialize(const Tag& tag, YAML::Emitter& out) {
        out << YAML::Key << "Tag" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << tag.getName();
        out << YAML::Key << "UUID" << YAML::Value << tag.getUuid();
        out << YAML::EndMap;
    }

    Tag Tag::deserialize(const YAML::Node& node) {
        return Tag(node["Name"].as<std::string>(), node["UUID"].as<std::string>());
    }

    void Transform::serialize(const Transform& transform, YAML::Emitter& out) {
        out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.position.x << transform.position.y << transform.position.z << YAML::EndSeq;
        out << YAML::Key << "Rotation" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.rotation.x << transform.rotation.y << transform.rotation.z << YAML::EndSeq;
        out << YAML::Key << "Scale" << YAML::Value << YAML::Flow << YAML::BeginSeq << transform.scale.x << transform.scale.y << transform.scale.z << YAML::EndSeq;
        out << YAML::EndMap;
    }

    Transform Transform::deserialize(const YAML::Node& node) {
        Transform transform;
        transform.position = glm::vec3(node["Position"].as<glm::vec3>());
        transform.rotation = glm::vec3(node["Rotation"].as<glm::vec3>());
        transform.scale = glm::vec3(node["Scale"].as<glm::vec3>());
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
        meshRenderer.paintMesh();
        return meshRenderer;
    }
}
