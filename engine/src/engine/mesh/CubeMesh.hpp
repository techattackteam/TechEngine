#pragma once

#include "Mesh.hpp"

namespace Engine {

    class CubeMesh : public Mesh {
    public:
        CubeMesh();

        void createMesh() override;

        void createQuad(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, glm::vec4 color);

    };
}


