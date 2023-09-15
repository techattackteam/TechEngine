#pragma once

#include "Mesh.hpp"

namespace TechEngine {

    class CubeMesh : public Mesh {
    public:
        CubeMesh();

        void createMesh() override;

        std::string getName() override;
    };
}


