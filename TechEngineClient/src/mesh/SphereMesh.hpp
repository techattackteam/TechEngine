#pragma once

#include "Mesh.hpp"

namespace TechEngine {
    class SphereMesh : public Mesh {
    private:
        std::string name = "SphereMesh";

        const float radius = 0.5f;
        const float sectors = 32;
        const float stacks = 18;

    public:
        SphereMesh();

        void createMesh() override;

        std::string getName() override;
    };
}
