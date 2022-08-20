#include "CubeMesh.hpp"

namespace TechEngine {
    CubeMesh::CubeMesh() : Mesh() {
        createMesh();
    }

    void CubeMesh::createQuad(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4) {
        glm::vec3 normal = getTriangleNormals(v1, v2, v3);
        createVertex(v1, normal);
        createVertex(v4, normal);
        createVertex(v3, normal);

        createVertex(v1, normal);
        createVertex(v3, normal);
        createVertex(v2, normal);
    }

    void CubeMesh::createMesh() {
        float BLOCK_SIDE = 0.5f;
        glm::vec3 v1 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v2 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v3 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v4 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);

        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);

        v1 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);

        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);

        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);

        v1 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4);
    }
}
