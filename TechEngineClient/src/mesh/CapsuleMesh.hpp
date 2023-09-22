#pragma once

#include "Mesh.hpp"

namespace TechEngine {
    class CapsuleMesh : public Mesh {
    private:
        const int numSegments = 32;
        const int numStacks = 16;
        const float radius = 0.5f;
        const float height = 1.0f;
    public:
        CapsuleMesh();

        void createMesh() override;

        std::string getName() override;
    };
}
