#pragma once

#include "Mesh.hpp"

namespace TechEngine {
    class CylinderMesh : public Mesh {
    private:
        float radius = 0.5f;
        float height = 1.0f;
        const int numSegments = 128;

    public:
        CylinderMesh();

        std::string getName() override;
    };
}
