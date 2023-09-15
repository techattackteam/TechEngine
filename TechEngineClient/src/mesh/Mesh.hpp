#pragma once

#include <vector>
#include "../renderer/VertexBuffer.hpp"
#include "components/TransformComponent.hpp"

namespace TechEngine {
    class Mesh {
    private:
        std::vector<Vertex> vertices;
        std::vector<int> indices;
    public:
        Mesh();

        virtual ~Mesh();

        virtual void createMesh() = 0;

        std::vector<Vertex> &getVertices();

        std::vector<int> getIndices();

        virtual std::string getName() = 0;

    protected:
        glm::vec3 getTriangleNormals(glm::vec3 vec1, glm::vec3 vec2, glm::vec3 vec3);

        void createVertex(glm::vec3 position, glm::vec3 normal);

        void createIndex(int index);

    };
}


