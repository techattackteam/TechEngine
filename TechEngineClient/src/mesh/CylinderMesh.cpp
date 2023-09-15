#include "CylinderMesh.hpp"

namespace TechEngine {

    CylinderMesh::CylinderMesh() : Mesh() {
        createMesh();

    }

    void CylinderMesh::createMesh() {
        const float segmentAngle = glm::two_pi<float>() / static_cast<float>(segments);

        // Create the vertices and normals for the side of the cylinder
        for (int i = 0; i <= segments; ++i) {
            float angle = i * segmentAngle;
            float x = radius * glm::cos(angle);
            float z = radius * glm::sin(angle);

            // Create two vertices for the top and bottom points of the cylinder side
            glm::vec3 topVertex(x, height * 0.5f, z);
            glm::vec3 bottomVertex(x, -height * 0.5f, z);

            // Compute normals (outward for the sides)
            glm::vec3 sideNormal = glm::normalize(glm::vec3(x, 0.0f, z));

            createVertex(topVertex, sideNormal);
            createVertex(bottomVertex, sideNormal);
        }

        // Create the vertices and normals for the top and bottom faces
        glm::vec3 topCenter(0.0f, height * 0.5f, 0.0f);
        glm::vec3 bottomCenter(0.0f, -height * 0.5f, 0.0f);

        // Compute normals for the top and bottom faces (upward and downward)
        glm::vec3 topNormal(0.0f, 1.0f, 0.0f);
        glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);

        createVertex(topCenter, topNormal);
        createVertex(bottomCenter, bottomNormal);

        // Create the indices to form the triangles
        for (int i = 0; i < segments * 2; i += 2) {
            // Indices for the first triangle (side)
            createIndex(i);
            createIndex(i + 1);
            createIndex(i + 2);

            // Indices for the second triangle (side)
            createIndex(i + 2);
            createIndex(i + 1);
            createIndex(i + 3);
        }

        // Create the indices to form the triangles
        for (int i = 0; i < segments * 2; i += 2) {
            // Indices for the first triangle (side, reversed winding order)
            createIndex(i + 2);
            createIndex(i + 1);
            createIndex(i);

            // Indices for the second triangle (side, reversed winding order)
            createIndex(i + 3);
            createIndex(i + 1);
            createIndex(i + 2);
        }

        // Create indices for the top and bottom faces
        for (int i = 0; i < segments; ++i) {
            createIndex(i * 2 + 2);     // Next top vertex
            createIndex(i * 2 + 1);     // Bottom vertex
            createIndex(i * 2);         // Top vertex

            createIndex(i * 2 + 3);     // Next bottom vertex
            createIndex(i * 2 + 1);     // Bottom vertex
            createIndex(i * 2 + 2);     // Next top vertex
        }
    }


    std::string CylinderMesh::getName() {
        return "Cylinder";
    }
}
