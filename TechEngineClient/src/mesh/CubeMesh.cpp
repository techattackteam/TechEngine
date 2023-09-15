#include "CubeMesh.hpp"

namespace TechEngine {
    CubeMesh::CubeMesh() : Mesh() {
        createMesh();
    }

    void CubeMesh::createMesh() {
        createVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0, 0, 1));
        createVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, 0, 1));
        createVertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 0, 1));
        createVertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0, 0, 1));

        createVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0, 0, -1));
        createVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, 0, -1));
        createVertex(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 0, -1));
        createVertex(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0, 0, -1));

        createIndex(0);
        createIndex(1);
        createIndex(3);
        createIndex(3);
        createIndex(1);
        createIndex(2);

        createIndex(1);
        createIndex(5);
        createIndex(2);
        createIndex(2);
        createIndex(5);
        createIndex(6);

        createIndex(5);
        createIndex(4);
        createIndex(6);
        createIndex(6);
        createIndex(4);
        createIndex(7);

        createIndex(4);
        createIndex(0);
        createIndex(7);
        createIndex(7);
        createIndex(0);
        createIndex(3);

        createIndex(3);
        createIndex(2);
        createIndex(7);
        createIndex(7);
        createIndex(2);
        createIndex(6);

        createIndex(4);
        createIndex(5);
        createIndex(0);
        createIndex(0);
        createIndex(5);
        createIndex(1);
    }

    std::string CubeMesh::getName() {
        return "Cube";
    }
}
