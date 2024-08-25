#pragma once
#include <glm/glm.hpp>
#include <utility>
#include <string>

#include "material/Material.hpp"
#include "mesh/Mesh.hpp"

namespace TechEngine {
    class Tag {
    public:
        const std::string uuid;
        std::string name;

        explicit Tag(std::string name, std::string tag) : name(std::move(name)), uuid(std::move(tag)) {
        }
    };

    class Transform {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // Maybe use quaternions instead
        glm::vec3 scale = glm::vec3(1.0f);
    };

    class Camera {
    public:
        bool isMainCamera = false;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        float fov = 90;
        float nearPlane = 0.1f;
        float farPlane = 100;
        float orthoSize = 5;

        glm::mat4 getProjectionMatrix() const {
            return projectionMatrix;
        }

        glm::mat4 getViewMatrix() const {
            return viewMatrix;
        }

        void updateProjectionMatrix(float aspectRatio) {
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }
    };

    class MeshRenderer {
    public:
        Mesh& mesh;
        Material& material;

        std::vector<Vertex> getVertices() {
            return mesh.vertices;
        }

        std::vector<int> getIndices() {
            return mesh.indices;
        }
    };
}
