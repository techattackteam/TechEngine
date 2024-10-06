#pragma once
#include <typeindex>
#include <yaml-cpp/emitter.h>

#include "resources/material/Material.hpp"
#include "resources/mesh/Mesh.hpp"

namespace TechEngine {
    class ResourcesManager;
    using Entity = int32_t;
    using ComponentTypeID = uint32_t;

    class CORE_DLL Tag {
    private:
        char* uuid;
        char* name;

    public:
        Tag(const std::string& name, const std::string& uuid): name(new char[name.size() + 1]), uuid(new char[uuid.size() + 1]) {
            strcpy_s(this->name, name.size() + 1, name.c_str());
            strcpy_s(this->uuid, uuid.size() + 1, uuid.c_str());
            //Memory leak here because I'm not deleting the memory allocated for name and uuid when deleting entity
        }

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

        //Copy constructor
        Camera(const Camera& camera) {
            mainCamera = camera.mainCamera;
            viewMatrix = camera.viewMatrix;
            projectionMatrix = camera.projectionMatrix;
            fov = camera.fov;
            nearPlane = camera.nearPlane;
            farPlane = camera.farPlane;
            orthoSize = camera.orthoSize;
            aspectRatio = camera.aspectRatio;
            TE_LOGGER_INFO("Camera copied");
        }

        //Move constructor
        Camera(Camera&& camera) noexcept {
            mainCamera = camera.mainCamera;
            viewMatrix = camera.viewMatrix;
            projectionMatrix = camera.projectionMatrix;
            fov = camera.fov;
            nearPlane = camera.nearPlane;
            farPlane = camera.farPlane;
            orthoSize = camera.orthoSize;
            aspectRatio = camera.aspectRatio;
            TE_LOGGER_INFO("Camera moved");
        }

        //Copy assignment
        Camera& operator=(const Camera& camera) {
            if (this == &camera) {
                return *this;
            }
            mainCamera = camera.mainCamera;
            viewMatrix = camera.viewMatrix;
            projectionMatrix = camera.projectionMatrix;
            fov = camera.fov;
            nearPlane = camera.nearPlane;
            farPlane = camera.farPlane;
            orthoSize = camera.orthoSize;
            aspectRatio = camera.aspectRatio;
            TE_LOGGER_INFO("Camera copied");
            return *this;
        }

        //Move assignment
        Camera& operator=(Camera&& camera) noexcept {
            if (this == &camera) {
                return *this;
            }
            mainCamera = camera.mainCamera;
            viewMatrix = camera.viewMatrix;
            projectionMatrix = camera.projectionMatrix;
            fov = camera.fov;
            nearPlane = camera.nearPlane;
            farPlane = camera.farPlane;
            orthoSize = camera.orthoSize;
            aspectRatio = camera.aspectRatio;
            TE_LOGGER_INFO("Camera moved");
            return *this;
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
