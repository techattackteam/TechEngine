#pragma once
#include "TechEngine/core/resources/material/Material.hpp"
#include "TechEngine/core/resources/mesh/Mesh.hpp"


#include "TechEngine/core/core/CoreExportDLL.hpp"

#include <typeindex>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Entity.hpp"

namespace YAML {
    class Emitter;
    class Node;
}

namespace TechEngine {
    class ResourcesManager;
    class PhysicsEngine;

    class CORE_DLL Tag {
        friend class ComponentsFactory;

    private:
        char* uuid = nullptr;
        char* name = nullptr;

    public:
        Tag() = default;

        ~Tag() {
            delete[] uuid;
            delete[] name;
        }

        Tag(const Tag& other) {
            if (other.uuid) {
                uuid = new char[strlen(other.uuid) + 1];
                strcpy_s(uuid, strlen(other.uuid) + 1, other.uuid);
            }
            if (other.name) {
                name = new char[strlen(other.name) + 1];
                strcpy_s(name, strlen(other.name) + 1, other.name);
            }
        }

        Tag(Tag&& other) noexcept {
            uuid = other.uuid;
            name = other.name;
            other.uuid = nullptr;
            other.name = nullptr;
        }

        Tag& operator=(const Tag& other) {
            if (this != &other) {
                delete[] uuid;
                delete[] name;
                uuid = nullptr;
                name = nullptr;
                if (other.uuid) {
                    uuid = new char[strlen(other.uuid) + 1];
                    strcpy_s(uuid, strlen(other.uuid) + 1, other.uuid);
                }
                if (other.name) {
                    name = new char[strlen(other.name) + 1];
                    strcpy_s(name, strlen(other.name) + 1, other.name);
                }
            }
            return *this;
        }

        Tag& operator=(Tag&& other) noexcept {
            if (this != &other) {
                delete[] uuid;
                delete[] name;
                uuid = other.uuid;
                name = other.name;
                other.uuid = nullptr;
                other.name = nullptr;
            }
            return *this;
        }

        bool operator==(const Tag& lhr) const {
            if (uuid == nullptr || lhr.uuid == nullptr) {
                return uuid == lhr.uuid;
            }
            return strcmp(uuid, lhr.uuid) == 0;
        }

        [[nodiscard]] std::string getName() const {
            return name ? name : "";
        }

        [[nodiscard]] std::string getUuid() const {
            return uuid ? uuid : "";
        }

        void setName(const std::string& name) {
            delete[] this->name;
            this->name = new char[name.size() + 1];
            strcpy_s(this->name, name.size() + 1, name.c_str());
        }

        static void serialize(const Tag& tag, YAML::Emitter& out);

        static Tag deserialize(const YAML::Node& node);
    };

    class CORE_DLL Hierarchy {
    public:
        Entity parent = -1;
        Entity firstChild = -1;
        Entity nextSibling = -1;
        Entity previousSibling = -1;

        size_t childrenCount = 0;

        static void serialize(const Hierarchy& hierarchy, YAML::Emitter& out);

        static Hierarchy deserialize(const YAML::Node& node);
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

        glm::mat4 m_worldMatrix = glm::mat4(1.0f);
        bool m_isDirty = true;


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
            glm::quat quaternion = glm::quat(glm::radians(m_rotation));

            m_forward = glm::normalize(quaternion * glm::vec3(0.0f, 0.0f, -1.0f));
            m_right = glm::normalize(quaternion * glm::vec3(1.0f, 0.0f, 0.0f));
            m_up = glm::normalize(quaternion * glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 getLocalModelMatrix() {
            return glm::translate(glm::mat4(1), m_position) *
                   glm::mat4_cast(glm::quat(glm::radians(m_rotation))) *
                   glm::scale(glm::mat4(1), m_scale);
        }

        glm::mat4 getModelMatrix() {
            return m_worldMatrix;
        }

        static void serialize(const Transform& transform, YAML::Emitter& out);

        static Transform deserialize(const YAML::Node& node);
    };

    class CORE_DLL Camera {
    public:
        bool mainCamera = false;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);


        float fov = 45;
        float nearPlane = 0.1f;
        float farPlane = 500;
        float orthoSize = 20.0f;
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
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
            //projectionMatrix = glm::ortho(-orthoSize * aspectRatio / 2.0f, orthoSize * aspectRatio / 2.0f, -orthoSize / 2.0f, orthoSize / 2.0f, nearPlane, farPlane);
            this->aspectRatio = aspectRatio;
        }

        bool isMainCamera() const {
            return mainCamera;
        }

        float getFov() const {
            return fov;
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

        // This changes the mesh but does not notify the Renderer about this change since it does not have access to the Event System
        void changeMesh(Mesh& mesh) {
            this->mesh = &mesh;
        }

        static void serialize(const MeshRenderer& meshRenderer, YAML::Emitter& out);

        static MeshRenderer deserialize(const YAML::Node& node, ResourcesManager& resourcesManager);
    };

#pragma region Light Components
    class CORE_DLL PointLight {
    public:
        glm::vec3 color = glm::vec3(1.0); // 12 bytes
        float radius = 10; // 4 byte
        float intensity = 1; // 4 byte
        float padding3[3] = {0.0f}; // Padding to align to 16 bytes

        uint32_t gpuID = -1; // ID in the GPU
        bool castShadows = true;

        uint32_t shadowMapID = -1;
        uint64_t shadowTextureHandle = 0.0f;

        static void serialize(const PointLight& staticBody, YAML::Emitter& out);

        static PointLight deserialize(const YAML::Node& node);
    };

    class CORE_DLL DirectionalLight {
    public:
        // Direction is from the entity's Transform component
        glm::vec3 color = glm::vec3(1.0); // 12 bytes
        float intensity = 1;

        uint32_t gpuID = -1; // ID in the GPU
        bool castShadows = true;

        uint32_t shadowMapID[4] = {};
        uint64_t shadowTextureHandle[4] = {};
        glm::mat4 lightSpaceMatrix[4] = {glm::mat4(1.0f)};
        float cascadeSplits[4] = {0.0f};

        static void serialize(const DirectionalLight& directionalLight, YAML::Emitter& out);

        static DirectionalLight deserialize(const YAML::Node& node);
    };

    class CORE_DLL SpotLight {
    public:
        glm::vec3 color = glm::vec3(1.0);
        float intensity = 1.0f;
        float innerCutoff = 12.5f;
        float outerCutoff = 17.5f;

        uint32_t gpuID = -1;
        bool castShadows = true;

        uint32_t shadowMapID = -1;
        uint64_t shadowTextureHandle = 0.0f;
        glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

        static void serialize(const SpotLight& spotLight, YAML::Emitter& out);

        static SpotLight deserialize(const YAML::Node& node);
    };
#pragma endregion

#pragma region Physics Components
    class CORE_DLL StaticBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

        static void serialize(const StaticBody& staticBody, YAML::Emitter& out);

        static StaticBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL KinematicBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

        static void serialize(const KinematicBody& kinematicBody, YAML::Emitter& out);

        static KinematicBody deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL RigidBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

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
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 size = glm::vec3(1.0f);

        static void serialize(const BoxTrigger& boxTrigger, YAML::Emitter& out);

        static BoxTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 0.5f;

        static void serialize(const SphereTrigger& sphereCollider, YAML::Emitter& out);

        static SphereTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CapsuleTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(const CapsuleTrigger& capsuleCollider, YAML::Emitter& out);

        static CapsuleTrigger deserialize(const YAML::Node& node, PhysicsEngine& m_physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CylinderTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
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
}
