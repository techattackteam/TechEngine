#pragma once
#include "resources/material/Material.hpp"
#include "resources/mesh/Mesh.hpp"

#include <typeindex>
#include <yaml-cpp/emitter.h>
#include <Jolt/Jolt.h>

namespace TechEngine {
    class ResourcesManager;
    class PhysicsEngine;


    class CORE_DLL Tag {
        friend class ComponentsFactory;
        char* uuid;
        char* name;

        /*Tag(const std::string& name, const std::string& uuid) : name(new char[name.size() + 1]), uuid(new char[uuid.size() + 1]) {
            strcpy_s(this->name, name.size() + 1, name.c_str());
            strcpy_s(this->uuid, uuid.size() + 1, uuid.c_str());
            //Memory leak here because I'm not deleting the memory allocated for name and uuid when deleting entity
        }*/

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
    public:
        friend class ComponentsFactory;

        Transform() {
            calculateUpForwardRight();
        }

    public:
        glm::vec3 m_position = glm::vec3(0.0f);
        glm::vec3 m_rotation = glm::vec3(0.0f); // Maybe use quaternions instead
        glm::vec3 m_scale = glm::vec3(1.0f);
        glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);


        void translate(glm::vec3 vector) {
            m_position += vector;
        }

        void translateTo(glm::vec3 position) {
            m_position = position;
        }

        void translateToWorld(glm::vec3 worldPosition) {
            m_position = worldPosition;
        }

        void setRotation(glm::vec3 rotation) {
            m_rotation = rotation;
            calculateUpForwardRight();
        }

        void rotate(glm::vec3 rotation) {
            m_rotation += rotation;
            calculateUpForwardRight();
        }

        void setRotation(glm::quat quaternion) {
            m_rotation = glm::eulerAngles(quaternion);
            calculateUpForwardRight();
        }

        void setScale(glm::vec3 vector) {
            m_scale = vector;
        }


        void calculateUpForwardRight() {
            glm::vec3 forward = glm::normalize(glm::vec3(
                -sin(glm::radians(m_rotation.y)) * cos(glm::radians(m_rotation.x)),
                sin(glm::radians(m_rotation.x)),
                -cos(glm::radians(m_rotation.y)) * cos(glm::radians(m_rotation.x))
            ));
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); // World Up
            glm::vec3 up = glm::normalize(glm::cross(right, forward));
            m_forward = forward;
            m_up = up;
            m_right = right;
        }

        glm::mat4 getModelMatrix() {
            return glm::translate(glm::mat4(1), m_position) *
                   glm::mat4_cast(glm::quat(glm::radians(m_rotation))) *
                   glm::scale(glm::mat4(1), m_scale);
        }

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
        float farPlane = 10000;
        float orthoSize = 5;
        float aspectRatio = 1;
        friend class CameraSystem;

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

    class CORE_DLL MeshRenderer {
    public:
        //TODO: Change to mesh and material IDs and get them from ResourcesManager
        Mesh* mesh;
        Material* material;


        std::vector<Vertex> getVertices() const {
            return mesh->m_vertices;
        }

        std::vector<int> getIndices() const {
            return mesh->m_indices;
        }

        void changeMaterial(Material& material) {
            this->material = &material;
        }

        void changeMesh(Mesh& mesh) {
            this->mesh = &mesh;
        }

        static void serialize(const MeshRenderer& meshRenderer, YAML::Emitter& out);

        static MeshRenderer deserialize(const YAML::Node& node, ResourcesManager& resourcesManager);
    };
#pragma region Physics Components
    class CORE_DLL StaticBody {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;

        static void serialize(const StaticBody& staticBody, YAML::Emitter& out);

        static StaticBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL KinematicBody {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;

        static void serialize(const KinematicBody& kinematicBody, YAML::Emitter& out);

        static KinematicBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL RigidBody {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;

        static void serialize(const RigidBody& rigidbody, YAML::Emitter& out);

        static RigidBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL BoxCollider {
    private:
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 size = glm::vec3(1.0f);

        static void serialize(const BoxCollider& boxCollider, YAML::Emitter& out);

        static BoxCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 0.5f;

        static void serialize(const SphereCollider& sphereCollider, YAML::Emitter& out);

        static SphereCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CapsuleCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(const CapsuleCollider& capsuleCollider, YAML::Emitter& out);

        static CapsuleCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CylinderCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(const CylinderCollider& cylinderCollider, YAML::Emitter& out);

        static CylinderCollider deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL BoxTrigger {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 size = glm::vec3(1.0f);

        static void serialize(const BoxTrigger& boxTrigger, YAML::Emitter& out);

        static BoxTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereTrigger {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 0.5f;

        static void serialize(const SphereTrigger& sphereCollider, YAML::Emitter& out);

        static SphereTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CapsuleTrigger {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(const CapsuleTrigger& capsuleCollider, YAML::Emitter& out);

        static CapsuleTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CylinderTrigger {
        friend class ComponentsFactory;

    public:
        JPH::uint32 index;
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(const CylinderTrigger& cylinderCollider, YAML::Emitter& out);

        static CylinderTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };
#pragma endregion

#pragma region Audio Components
    class CORE_DLL AudioListener {
        static void serialize(const AudioListener& audioListener, YAML::Emitter& out);

        static AudioListener deserialize(const YAML::Node& node);
    };

    class CORE_DLL AudioEmitter {
    public:
        float volume = 1.0f; // Volume of the emitter
        float pitch = 1.0f; // Pitch of the emitter
        bool loop = false; // Whether the emitter should loop the sound
        //std::string soundPath; // Path to the sound file

        static void serialize(const AudioEmitter& emitter, YAML::Emitter& out);

        static AudioEmitter deserialize(const YAML::Node& node);
    };
#pragma endregion
    /*class CORE_DLL ComponentType {
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
            registerComponent<BoxCollider>();
            registerComponent<SphereCollider>();
            registerComponent<CapsuleCollider>();
            registerComponent<CylinderCollider>();
            registerComponent<BoxTrigger>();
            registerComponent<SphereTrigger>();
            registerComponent<CapsuleTrigger>();
            registerComponent<CylinderTrigger>();
            registerComponent<AudioEmitterComponent>();
            registerComponent<AudioListenerComponent>();
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
    };*/
}
