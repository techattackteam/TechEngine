#include "CylinderMesh.hpp"

namespace TechEngine {

    CylinderMesh::CylinderMesh() : Mesh() {
        createMesh();

    }


    void CylinderMesh::createMesh() {
        float points_top[] = {
                0.0f, radius * 2.0f, 0.0f,
        };

        float points_bottom[] = {
                0.0f, -radius * 2.0f, 0.0f,
        };

        int numP = 1;
        int numSides = numSegments;

        float inc = 2 * 3.14159f / (numSides);
        float nx = 1, ny = 1;
        float delta;
        for (int i = 0; i < numP; i++) {
            for (int j = 0; j <= numSides; j++) {
                delta = 0.0f;
                createVertex(glm::vec3(points_top[i * 3] * cos(j * inc), points_top[i * 3 + 1], points_top[i * 3] * sin(-j * inc)),
                             glm::vec3(nx * cos(j * inc + delta), ny, nx * sin(-j * inc + delta)), glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        for (int i = 0; i < numP; i++) {
            for (int j = 0; j <= numSides; j++) {
                delta = 0.0f;
                createVertex(glm::vec3(points_bottom[i * 3] * cos(j * inc), points_bottom[i * 3 + 1], points_bottom[i * 3] * sin(-j * inc)),
                             glm::vec3(nx * cos(j * inc + delta), ny, nx * sin(-j * inc + delta)), glm::vec3(1.0f, 0.0f, 0.0f));
            }
        }

        for (int i = 0, k = 0; i < numP - 1; i++, k++) {
            for (int j = 0; j < numSides; j++) {
                createIndex(k * (numSides + 1) + j);
                createIndex((k + 1) * (numSides + 1) + j + 1);
                createIndex((k + 1) * (numSides + 1) + j);
                createIndex(k * (numSides + 1) + j);
                createIndex(k * (numSides + 1) + j + 1);
                createIndex((k + 1) * (numSides + 1) + j + 1);
            }
        }

        int baseCenterIndex = (numSides + 1) * numP;
        int topCenterIndex = baseCenterIndex + (numSides + 1);

        for (int i = 0; i < numSides; ++i) {
            int k1 = i;
            int k2 = k1 + 1;
            int k3 = k1 + numSides + 1;
            int k4 = k2 + numSides + 1;

            // Triangle 1
            createIndex(k1);
            createIndex(k2);
            createIndex(k4);

            // Triangle 2
            createIndex(k1);
            createIndex(k4);
            createIndex(k3);
        }

// Create indices for the top and bottom faces
        for (int i = 0; i < numSides; ++i) {
            int k1 = i;
            int k2 = i + 1;

            // Top face
            createIndex(topCenterIndex);
            createIndex(k1);
            createIndex(k2);

            // Bottom face
            createIndex(baseCenterIndex);
            createIndex(k2 + numSides + 1);
            createIndex(k1 + numSides + 1);
        }

    }

    std::string CylinderMesh::getName() {
        return "Cylinder";
    }
}