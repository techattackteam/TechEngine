#pragma once

#include "core/CoreExportDll.hpp"
#include "Vertex.hpp"
#include <string>
#include <vector>

namespace TechEngine {
    class CORE_DLL Mesh {
    private:
        std::vector<Vertex> vertices;
        std::vector<int> indices;

    public:
        Mesh();

        Mesh(std::vector<Vertex> vertices, std::vector<int> indices);

        virtual ~Mesh();

        std::vector<Vertex>& getVertices();

        std::vector<int> getIndices();

        virtual std::string getName();

    protected:
        glm::vec3 getTriangleNormals(glm::vec3 vec1, glm::vec3 vec2, glm::vec3 vec3);

        void createVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 textureCoordinate);

        void createIndex(int index);
    };
}
