#pragma once

#include <vector>
#include "../renderer/VertexBuffer.hpp"
#include "../components/TransformComponent.hpp"

namespace TechEngine {
    class Mesh {
    private:
        std::vector<Vertex> vertices;

    public:
        Mesh();

        virtual void createMesh() = 0;

        std::vector<Vertex> &getVertices();

    protected:
        glm::vec3 getTriangleNormals(glm::vec3 vec1, glm::vec3 vec2, glm::vec3 vec3);

        void createVertex(glm::vec3 offset, glm::vec3 normal);
    };
}


