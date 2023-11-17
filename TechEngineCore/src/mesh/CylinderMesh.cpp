#include "CylinderMesh.hpp"

namespace TechEngine {

    CylinderMesh::CylinderMesh() : Mesh() {
        createMesh();
    }

    void CylinderMesh::createMesh() {
        float points[] = {
                -radius, -height * 0.5f,
                0.0f, -height * 0.5f,
                radius, -height * 0.5f,
                radius, height * 0.5f,
                0.0f, height * 0.5f,
                -radius, height * 0.5f
        };
        float *p = points + 2;
        int numP = 4;
        int numSides = numSegments;

        float inc = 2 * 3.14159f / numSides;
        float nx = 1, ny = 1;
        float delta;

        for (int i = 0, k = 0; i < numP; i++, k++) {
            for (int j = 0; j <= numSides; j++) {
                if ((i == 0 && p[0] == 0.0f) || (i == numP - 1 && p[(i + 1) * 2] == 0.0))
                    delta = inc * 0.5f;
                else
                    delta = 0.0f;

                float x = p[i * 2] * cos(j * inc);
                float z = p[i * 2] * sin(-j * inc);

                float u = static_cast<float>(j) / static_cast<float>(numSides);
                float v = static_cast<float>(i) / static_cast<float>(numP - 1);

                createVertex(glm::vec3(x, p[(i * 2) + 1], z), glm::vec3(nx * cos(j * inc + delta), ny, nx * sin(-j * inc + delta)), glm::vec2(u, v));
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
    }

    std::string CylinderMesh::getName() {
        return "Cylinder";
    }
}
