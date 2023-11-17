#include "SphereMesh.hpp"
#include "core/Logger.hpp"

namespace TechEngine {

    SphereMesh::SphereMesh() : Mesh() {
        createMesh();
    }

    void SphereMesh::createMesh() {
        float sectorStep = 2 * glm::pi<float>() / sectors;
        float stackStep = glm::pi<float>() / stacks;
        float sectorAngle, stackAngle;
        float x, y, z; // vertex position

        for (int i = 0; i <= stacks; ++i) {
            stackAngle = glm::pi<float>() / 2 - i * stackStep; // starting from pi/2 to -pi/2
            float xz = radius * cosf(stackAngle); // r * cos(u)
            y = radius * sinf(stackAngle); // r * sin(u)

            for (int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xz * sinf(sectorAngle); // r * cos(u) * sin(v)
                z = xz * cosf(sectorAngle); // r * cos(u) * cos(v)

                glm::vec3 position(x, y, z);
                glm::vec3 normal(x / radius, y / radius, z / radius);

                // Calculate texture coordinates
                float s = static_cast<float>(j) / static_cast<float>(sectors);
                float t = static_cast<float>(i) / static_cast<float>(stacks);

                glm::vec2 texCoord(s, t);

                createVertex(position, normal, texCoord);
            }
        }
        for (int i = 0; i < stacks; ++i) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;

            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                // Two triangles per sector
                if (i != 0) {
                    createIndex(k1);
                    createIndex(k2);
                    createIndex(k1 + 1);
                }

                if (i != stacks - 1) {
                    createIndex(k1 + 1);
                    createIndex(k2);
                    createIndex(k2 + 1);
                }
            }
        }
    }

    std::string SphereMesh::getName() {
        return "Sphere";
    }
}
