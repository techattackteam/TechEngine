#include "MeshComponent.hpp"

namespace Engine {

    MeshComponent::MeshComponent() : vertices(), Component("Mesh") {

    }

    void MeshComponent::createVertex(glm::vec3 localPosition, glm::vec3 offset, glm::vec3 normal, glm::vec4 color) {
        Vertex vertex = Vertex(localPosition, normal, color);
        vertex.position += offset;
        vertices.push_back(vertex);
    }

    std::vector<Vertex> MeshComponent::getVertices() {
        return vertices;
    }

    void MeshComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }

    void MeshComponent::createCube() {
        glm::vec3 position = glm::vec3(0, 0, 0);
        glm::vec4 color = glm::vec4(0, 1, 1, 1);
        glm::vec3 normal = glm::vec3(1, 1, 1);
        glm::vec3 v1 = glm::vec3(0.5f, 0.5f, 0.0f);
        glm::vec3 v2 = glm::vec3(0.5f, -0.5f, 0.0f);
        glm::vec3 v3 = glm::vec3(-0.5f, 0.5f, 0.0f);
        glm::vec3 v4 = glm::vec3(-0.5f, -0.5f, 0.0f);
        //glm::vec3 normal = getVerticesNormals(v1, v2, v3);
        createVertex(position, v3, normal, color);
        createVertex(position, v2, normal, color);
        createVertex(position, v1, normal, color);

        createVertex(position, v4, normal, color);
        createVertex(position, v3, normal, color);
        createVertex(position, v2, normal, color);
    }
}
