#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <utility>
#include <string>
#include <typeindex>

#include "resources/material/Material.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/Mesh.hpp"

namespace TechEngine {
    using Entity = uint32_t;
    using ComponentTypeID = uint32_t;
    using ArchetypeID = uint32_t;


    class CORE_DLL Tag {
    private:
        char* uuid;
        char* name;

    public:
        Tag(const std::string& name, const std::string& uuid): name(new char[name.size() + 1]), uuid(new char[uuid.size() + 1]) {
            strcpy_s(this->name, name.size() + 1, name.c_str());
            strcpy_s(this->uuid, uuid.size() + 1, uuid.c_str());
            //Memory leak here because we are not deleting the memory allocated for name and uuid when deleting entity
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
    };

    class CORE_DLL Transform {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // Maybe use quaternions instead
        glm::vec3 scale = glm::vec3(1.0f);
    };

    class CORE_DLL Camera {
    private:
        bool mainCamera = false;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        float fov = 90;
        float nearPlane = 0.1f;
        float farPlane = 100;
        float orthoSize = 5;
        friend class CameraSystem;

    public:
        Camera() {
            std::cout << "Camera created" << std::endl;
        }

        glm::mat4 getProjectionMatrix() const {
            return projectionMatrix;
        }

        glm::mat4 getViewMatrix() const {
            return viewMatrix;
        }

        void updateProjectionMatrix(float aspectRatio) {
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
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
    };

    class MeshRenderer {
    public:
        Mesh& mesh;
        Material& material;

        explicit MeshRenderer(Mesh& mesh, Material& material) : mesh(mesh), material(material) {
        }

        std::vector<Vertex> getVertices() {
            return mesh.m_vertices;
        }

        std::vector<int> getIndices() {
            return mesh.m_indices;
        }

        void paintMesh() {
            for (Vertex& vertex: mesh.m_vertices) {
                vertex.setColor(material.getColor());
            }
        }
    };


    class CORE_DLL ComponentType {
    private:
        inline static ComponentTypeID counter = 0;
        inline static std::unordered_map<std::type_index, ComponentTypeID> typeMap;
        inline static bool initialized = false;

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
            if (!initialized) {
                init();
            }
            if (typeMap.find(typeid(T)) == typeMap.end()) {
                TE_LOGGER_CRITICAL("Component type {0} not registered, please register it first", typeid(T).name());
            }
            return typeMap[typeid(T)];
        }
    };
}
    