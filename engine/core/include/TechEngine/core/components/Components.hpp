#pragma once
#include "TechEngine/core/resources/material/MaterialResource.hpp"
#include "TechEngine/core/resources/mesh/MeshResource.hpp"
#include "TechEngine/core/core/CoreExportDLL.hpp"

#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Entity.hpp"

namespace TechEngine {
    class StreamWriter;
    class StreamReader;
    class ResourceSystem;
    class PhysicsEngine;

    class CORE_DLL Tag {
        friend class ComponentsFactory;

    private:
        //char* uuid = nullptr;
        //char* name = nullptr;
        std::string uuid;
        std::string name;

    public:
        Tag() = default;

        ~Tag() {
            //delete[] uuid;
            //delete[] name;
        }


        [[nodiscard]] std::string getName() const {
            return name;
        }

        [[nodiscard]] std::string getUuid() const {
            return uuid;
        }

        void setName(const std::string& name) {
            this->name = name;
        }

        static void serialize(StreamWriter* writer, const Tag& tag);

        static Tag deserialize(StreamReader* reader);
    };

    class CORE_DLL Hierarchy {
    public:
        Entity parent = -1;
        Entity firstChild = -1;
        Entity nextSibling = -1;
        Entity previousSibling = -1;

        size_t childrenCount = 0;

        static void serialize(StreamWriter* writer, const Hierarchy& hierarchy);

        static Hierarchy deserialize(StreamReader* reader);
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

        static void serialize(StreamWriter* writer, const Transform& transform);

        static Transform deserialize(StreamReader* reader);
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

        static void serialize(StreamWriter* writer, const Camera& camera);

        static Camera deserialize(StreamReader* reader);
    };

    class CORE_DLL MeshRenderer {
    public:
        UUID meshUUID;
        UUID materialUUID;

        void changeMaterial(const UUID& materialUUID) {
            this->materialUUID = materialUUID;
        }

        // This changes the mesh but does not notify the Renderer about this change since it does not have access to the Event System
        void changeMesh(const UUID& meshUUID) {
            this->meshUUID = meshUUID;
        }

        static void serialize(StreamWriter* writer, const MeshRenderer& meshRenderer);

        static MeshRenderer deserialize(StreamReader* reader);
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

        static void serialize(StreamWriter* writer, const PointLight& pointLight);

        static PointLight deserialize(StreamReader* reader);
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

        static void serialize(StreamWriter* writer, const DirectionalLight& directionalLight);

        static DirectionalLight deserialize(StreamReader* reader);
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

        static void serialize(StreamWriter* writer, const SpotLight& spotLight);

        static SpotLight deserialize(StreamReader* reader);
    };
#pragma endregion

#pragma region Physics Components
    class CORE_DLL StaticBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

        static void serialize(StreamWriter* writer, const StaticBody& staticBody);

        static StaticBody deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL KinematicBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

        static void serialize(StreamWriter* writer, const KinematicBody& kinematicBody);

        static KinematicBody deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL RigidBody {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;

        static void serialize(StreamWriter* writer, const RigidBody& rigidBody);

        static RigidBody deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL BoxCollider {
    private:
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 size = glm::vec3(1.0f);

        static void serialize(StreamWriter* writer, const BoxCollider& boxCollider);

        static BoxCollider deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const SphereCollider& sphereCollider);

        static SphereCollider deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CapsuleCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const CapsuleCollider& capsuleCollider);

        static CapsuleCollider deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CylinderCollider {
        friend class ComponentsFactory;

    public:
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const CylinderCollider& cylinderCollider);

        static CylinderCollider deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL BoxTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f, 0, 0);
        glm::vec3 size = glm::vec3(1.0f);

        static void serialize(StreamWriter* writer, const BoxTrigger& boxTrigger);

        static BoxTrigger deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL SphereTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f);
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const SphereTrigger& sphereTrigger);

        static SphereTrigger deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CapsuleTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const CapsuleTrigger& capsuleTrigger);

        static CapsuleTrigger deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };

    class CORE_DLL CylinderTrigger {
        friend class ComponentsFactory;

    public:
        std::uint32_t index;
        glm::vec3 center = glm::vec3(0.0f);
        float height = 1.0f;
        float radius = 0.5f;

        static void serialize(StreamWriter* writer, const CylinderTrigger& cylinderTrigger);

        static CylinderTrigger deserialize(StreamReader* reader, PhysicsEngine& physicsEngine, const Tag& tag, const Transform& transform);
    };
#pragma endregion

#pragma region Audio Components
    class CORE_DLL AudioListener {
    public:
        static void serialize(StreamWriter* writer, const AudioListener& audioListener);

        static AudioListener deserialize(StreamReader* reader);
    };

    class CORE_DLL AudioEmitter {
    public:
        float volume = 1.0f;
        float pitch = 1.0f;
        bool loop = false;

        static void serialize(StreamWriter* writer, const AudioEmitter& emitter);

        static AudioEmitter deserialize(StreamReader* reader);
    };
#pragma endregion
}
