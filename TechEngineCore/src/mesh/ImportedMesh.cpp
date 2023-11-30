#include <string>
#include "ImportedMesh.hpp"

namespace TechEngine {
    ImportedMesh::ImportedMesh(std::vector<Vertex> vertices, std::vector<int> indices) : Mesh(vertices, indices) {

    }

    std::string ImportedMesh::getName() {
        return "ImportedMesh";
    }


}
