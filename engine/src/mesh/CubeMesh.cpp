#include "CubeMesh.hpp"

namespace Engine {
    CubeMesh::CubeMesh() : Mesh() {
        createMesh();
    }

    void CubeMesh::createQuad(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, glm::vec4 color) {
        glm::vec3 normal = getTriangleNormals(v1, v2, v3);
        createVertex(v1, normal, color);
        createVertex(v4, normal, color);
        createVertex(v3, normal, color);

        createVertex(v1, normal, color);
        createVertex(v3, normal, color);
        createVertex(v2, normal, color);
    }

    void CubeMesh::createMesh() {
        float BLOCK_SIDE = 0.5f;
        glm::vec4 color = glm::vec4(0, 1, 1, 1);
        glm::vec3 v1 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v2 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v3 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        glm::vec3 v4 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);

        color = glm::vec4(1, 0, 1, 1);
        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);

        color = glm::vec4(1, 1, 0, 1);
        v1 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);

        color = glm::vec4(1, 0, 0, 1);
        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);

        color = glm::vec4(0, 1, 0, 1);
        v1 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(BLOCK_SIDE, BLOCK_SIDE, -BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);

        color = glm::vec4(0, 0, 1, 1);
        v1 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        v2 = glm::vec3(BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v3 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, -BLOCK_SIDE);
        v4 = glm::vec3(-BLOCK_SIDE, -BLOCK_SIDE, BLOCK_SIDE);
        createQuad(v1, v2, v3, v4, color);
    }
}
