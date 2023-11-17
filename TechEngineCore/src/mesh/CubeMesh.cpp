#include "CubeMesh.hpp"

namespace TechEngine {
    CubeMesh::CubeMesh() : Mesh() {
        createMesh();
    }

    void CubeMesh::createMesh() {
        // Define vertices with positions, normals, and texture coordinates
        // Front face
        createVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 0));
        createVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 0));
        createVertex(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(1, 1));
        createVertex(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0, 0, 1), glm::vec2(0, 1));

        // Back face
        createVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 0));
        createVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 0));
        createVertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(1, 1));
        createVertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1), glm::vec2(0, 1));

        // Top face
        createVertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0, 1, 0), glm::vec2(0, 0));
        createVertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 1, 0), glm::vec2(1, 0));
        createVertex(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 1, 0), glm::vec2(1, 1));
        createVertex(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0, 1, 0), glm::vec2(0, 1));

        // Bottom face
        createVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0, -1, 0), glm::vec2(0, 0));
        createVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, -1, 0), glm::vec2(1, 0));
        createVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, -1, 0), glm::vec2(1, 1));
        createVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0, -1, 0), glm::vec2(0, 1));

        // Left face
        createVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1, 0, 0), glm::vec2(0, 0));
        createVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1, 0, 0), glm::vec2(1, 0));
        createVertex(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-1, 0, 0), glm::vec2(1, 1));
        createVertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(-1, 0, 0), glm::vec2(0, 1));

        // Right face
        createVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1, 0, 0), glm::vec2(0, 0));
        createVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1, 0, 0), glm::vec2(1, 0));
        createVertex(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1, 0, 0), glm::vec2(1, 1));
        createVertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1, 0, 0), glm::vec2(0, 1));

        // Indices (order adjusted to fix the texture stretching issue)
        createIndex(0);
        createIndex(1);
        createIndex(2);
        createIndex(0);
        createIndex(2);
        createIndex(3);

        createIndex(4);
        createIndex(6);
        createIndex(5);
        createIndex(4);
        createIndex(7);
        createIndex(6);

        createIndex(8);
        createIndex(10);
        createIndex(9);
        createIndex(8);
        createIndex(11);
        createIndex(10);

        createIndex(12);
        createIndex(13);
        createIndex(14);
        createIndex(12);
        createIndex(14);
        createIndex(15);

        createIndex(16);
        createIndex(17);
        createIndex(18);
        createIndex(16);
        createIndex(18);
        createIndex(19);

        createIndex(20);
        createIndex(22);
        createIndex(21);
        createIndex(20);
        createIndex(23);
        createIndex(22);
    }

    std::string CubeMesh::getName() {
        return "Cube";
    }
}
