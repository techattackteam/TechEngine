#include "Mesh.hpp"

namespace Engine {

    Mesh::Mesh() : vertices() {
    }

    glm::vec3 Mesh::getTriangleNormals(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
        glm::vec3 edge1 = v1 - v2;
        glm::vec3 edge2 = v3 - v2;
        return glm::normalize(glm::cross(edge1, edge2));
    }

    void Mesh::createVertex(glm::vec3 vertexPosition, glm::vec3 normal, glm::vec4 color) {
        Vertex vertex = Vertex(vertexPosition, normal, color);
        vertices.push_back(vertex);
    }

    std::vector<Vertex> Mesh::getVertices() {
        return vertices;
    }
}