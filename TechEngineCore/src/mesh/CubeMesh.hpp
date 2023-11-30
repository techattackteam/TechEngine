#pragma once

#include "Mesh.hpp"

namespace TechEngine {

    class CubeMesh : public Mesh {
    public:
        CubeMesh();

        std::string getName() override;
    };
}


