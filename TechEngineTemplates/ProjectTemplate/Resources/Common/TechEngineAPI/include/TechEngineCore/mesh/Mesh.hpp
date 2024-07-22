#pragma once

#include "core/CoreExportDll.hpp"
#include "Vertex.hpp"
#include <string>
#include <vector>

namespace TechEngine {
    class CORE_DLL Mesh {
    private:
        std::string m_name;

        std::vector<Vertex> vertices;
        std::vector<int> indices;

    public:
        explicit Mesh(std::string name);

        Mesh(std::vector<Vertex> vertices, std::vector<int> indices);

        ~Mesh();

        std::vector<Vertex>& getVertices();

        std::vector<int> getIndices();

        std::string getName();

    private:
        glm::vec3 getTriangleNormals(glm::vec3 vec1, glm::vec3 vec2, glm::vec3 vec3);

        void createVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 textureCoordinate);

        void createIndex(int index);

        void createCubeMesh();

        void createSphereMesh();

        void createCylinderMesh();
    };
}
