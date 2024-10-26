#pragma once
#include "resources/material/Material.hpp"
#include "resources/mesh/Mesh.hpp"

#include <typeindex>
#include <yaml-cpp/emitter.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace TechEngine {
    class ResourcesManager;
    class PhysicsEngine;
    using Entity = int32_t;
    using ComponentTypeID = uint32_t;

    class CORE_DLL Tag {
        friend class ComponentsFactory;
        char* uuid;
        char* name;

        Tag(const std::string& name, const std::string& uuid): name(new char[name.size() + 1]), uuid(new char[uuid.size() + 1]) {
            strcpy_s(this->name, name.size() + 1, name.c_str());
            strcpy_s(this->uuid, uuid.size() + 1, uuid.c_str());
            //Memory leak here because I'm not deleting the memory allocated for name and uuid when deleting entity
        }

    public:
        bool operator==(const Tag& lhr) const {
            if (strcmp(uuid, lhr.uuid) != 0) {
                return false;
            } else {
                return true;
            }
        }

        [[nodiscard]] std::string getName() const {
            return name;
        }

        [[nodiscard]] std::string getUuid() const {
            return uuid;
        }

        void setName(const std::string& name) {
            delete[] this->name;
            this->name = new char[name.size() + 1];
            strcpy_s(this->name, name.size() + 1, name.c_str());
        }

        static void serialize(const Tag& tag, YAML::Emitter& out);

        static Tag deserialize(const YAML::Node& node);
    };

    class CORE_DLL Transform {
    private:
        friend class ComponentsFactory;

        Transform() = default;

    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // Maybe use quaternions instead
        glm::vec3 scale = glm::vec3(1.0f);

        static void serialize(const Transform& transform, YAML::Emitter& out);

        static Transform deserialize(const YAML::Node& node);
    };

    class CORE_DLL Camera {
    public:
        bool mainCamera = false;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);


        glm::vec1 fov = glm::vec1(45);
        float nearPlane = 0.1f;
        float farPlane = 100;
        float orthoSize = 5;
        float aspectRatio = 1;
        friend class CameraSystem;

    public:
        Camera() {
        }

        glm::mat4 getProjectionMatrix() const {
            return projectionMatrix;
        }

        glm::mat4 getViewMatrix() const {
            return viewMatrix;
        }

        void updateViewMatrix(const glm::mat4& modelMatrix) {
            viewMatrix = glm::inverse(modelMatrix);
        }

        void updateProjectionMatrix(float aspectRatio) {
            projectionMatrix = glm::perspective(glm::radians(fov.x), aspectRatio, nearPlane, farPlane);
            this->aspectRatio = aspectRatio;
        }

        bool isMainCamera() const {
            return mainCamera;
        }

        float getFov() const {
            return fov.x;
        }

        float getNearPlane() const {
            return nearPlane;
        }

        float getFarPlane() const {
            return farPlane;
        }

        float getOrthoSize() const {
            return orthoSize;
        }

        float getAspectRatio() const {
            return aspectRatio;
        }

        static void serialize(const Camera& camera, YAML::Emitter& out);

        static Camera deserialize(const YAML::Node& node);
    };

    class MeshRenderer {
    public:
        Mesh& mesh;
        Material& material;

        explicit MeshRenderer(Mesh& mesh, Material& material) : mesh(mesh), material(material) {
        }


        std::vector<Vertex> getVertices() const {
            return mesh.m_vertices;
        }

        std::vector<int> getIndices() const {
            return mesh.m_indices;
        }

        void paintMesh() {
            for (Vertex& vertex: mesh.m_vertices) {
                vertex.setColor(material.getColor());
            }
        }

        void changeMaterial(Material& material) {
            this->material = material;
            paintMesh();
        }

        void changeMesh(Mesh& mesh) {
            this->mesh = mesh;
            paintMesh();
        }

        static void serialize(const MeshRenderer& meshRenderer, YAML::Emitter& out);

        static MeshRenderer deserialize(const YAML::Node& node, ResourcesManager& resourcesManager);
    };

    class CORE_DLL BoxCollider {
    private:
        friend class ComponentsFactory;

        BoxCollider(glm::vec3 center, glm::vec3 scale) : center(center), scale(scale) {
        }

    public:
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 scale = glm::vec3(1.0f);

        static void serialize(const BoxCollider& boxCollider, YAML::Emitter& out);

        static BoxCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereCollider {
        friend class ComponentsFactory;

        SphereCollider() = default;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 1;

        static void serialize(const SphereCollider& boxCollider, YAML::Emitter& out);

        static SphereCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL StaticBody {
        friend class ComponentsFactory;

        explicit StaticBody(const JPH::BodyID& bodyID) : bodyID(bodyID) {
        };

    public:
        const JPH::BodyID& bodyID;

        static void serialize(const StaticBody& staticBody, YAML::Emitter& out);

        static StaticBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL KinematicBody {
        friend class ComponentsFactory;

        explicit KinematicBody(const JPH::BodyID& bodyID) : bodyID(bodyID) {
        };

    public:
        const JPH::BodyID& bodyID;

        static void serialize(const KinematicBody& kinematicBody, YAML::Emitter& out);

        static KinematicBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL RigidBody {
        friend class ComponentsFactory;

        explicit RigidBody(const JPH::BodyID& bodyID) : bodyID(bodyID) {
        };

    public:
        const JPH::BodyID& bodyID;

        static void serialize(const RigidBody& rigidbody, YAML::Emitter& out);

        static RigidBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL BoxTrigger {
        friend class ComponentsFactory;

        explicit BoxTrigger(const JPH::BodyID& bodyID) : bodyID(bodyID) {
        };

    public:
        const JPH::BodyID& bodyID;
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 scale = glm::vec3(1.0f);

        static void serialize(const BoxTrigger& boxTrigger, YAML::Emitter& out);

        static BoxTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL ComponentType {
    private:
        inline static ComponentTypeID counter = 0;
        inline static std::unordered_map<std::type_index, ComponentTypeID> typeMap;

    public:
        static void init() {
            registerComponent<Tag>();
            registerComponent<Transform>();
            registerComponent<Camera>();
            registerComponent<MeshRenderer>();
            registerComponent<StaticBody>();
            registerComponent<KinematicBody>();
            registerComponent<RigidBody>();
            registerComponent<BoxTrigger>();
            registerComponent<BoxCollider>();
            registerComponent<SphereCollider>();
        }

        template<typename T>
        static void registerComponent() {
            if (typeMap.find(typeid(T)) == typeMap.end()) {
                typeMap[typeid(T)] = counter;
                counter += 1;
            }
        }

        template<typename T>
        static bool isComponentRegistered() {
            return typeMap.find(typeid(T)) != typeMap.end();
        }

        template<typename T>
        static ComponentTypeID get() {
            if (typeMap.find(typeid(T)) == typeMap.end()) {
                TE_LOGGER_CRITICAL("Component type {0} not registered, please register it first", typeid(T).name());
            }
            return typeMap[typeid(T)];
        }
    };
}
