#pragma once

#include "Mesh.hpp"

namespace TechEngine {
    class CylinderMesh : public Mesh {
    private:
        float radius = 0.5f;
        float height = 1.0f;
        const int segments = 16;

    public:
        CylinderMesh();

        void createMesh() override;

        std::string getName() override;

    };
}
