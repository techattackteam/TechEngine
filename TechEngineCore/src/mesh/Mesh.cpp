#include "Mesh.hpp"

#include <stdexcept>
#include <glm/ext/scalar_constants.hpp>
#include <utility>

namespace TechEngine {
    Mesh::Mesh(std::string name) : m_name(std::move(name)), vertices() {
        if (m_name == "Cube") {
            createCubeMesh();
        } else if (m_name == "Sphere") {
            createSphereMesh();
        } else if (m_name == "Cylinder") {
            createCylinderMesh();
        } else {
            throw std::runtime_error("Mesh " + m_name + " is not supported.");
        }
    }

    Mesh::Mesh(std::vector<Vertex> vertices, std::vector<int> indices) {
        this->vertices = vertices;
        this->indices = indices;
    }

    Mesh::~Mesh() {
    }

    glm::vec3 Mesh::getTriangleNormals(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
        glm::vec3 edge1 = v1 - v2;
        glm::vec3 edge2 = v3 - v2;
        return glm::normalize(glm::cross(edge1, edge2));
    }

    void Mesh::createVertex(glm::vec3 vertexPosition, glm::vec3 normal, glm::vec2 textureCoordinate) {
        Vertex vertex = Vertex(vertexPosition, normal, textureCoordinate);
        vertices.push_back(vertex);
    }

    std::vector<Vertex>& Mesh::getVertices() {
        return vertices;
    }

    std::vector<int> Mesh::getIndices() {
        return indices;
    }

    void Mesh::createIndex(int index) {
        indices.push_back(index);
    }

    void Mesh::createCubeMesh() {
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

    void Mesh::createSphereMesh() {
        const float radius = 0.5f;
        const float sectors = 128;
        const float stacks = 128;
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

    void Mesh::createCylinderMesh() {
        float radius = 0.5f;
        float height = 1.0f;
        const int numSegments = 128;
        float points[] = {
            -radius, -height * 0.5f,
            0.0f, -height * 0.5f,
            radius, -height * 0.5f,
            radius, height * 0.5f,
            0.0f, height * 0.5f,
            -radius, height * 0.5f
        };
        float* p = points + 2;
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

    std::string Mesh::getName() {
        return m_name;
    }

}
