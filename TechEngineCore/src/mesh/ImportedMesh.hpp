#pragma once

#include "Mesh.hpp"

namespace TechEngine {
    class ImportedMesh : public Mesh {
    public:

        ImportedMesh(std::vector<Vertex> vertices, std::vector<int> indices);

        std::string getName() override;
    };

}
