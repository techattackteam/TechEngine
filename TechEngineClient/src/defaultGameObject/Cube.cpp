#include "Cube.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include <utility>
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Cube::Cube(Material* material) : GameObject("Cube") {
        addComponent<MeshRendererComponent>(new CubeMesh(), material);
    }
}
