#include "Cube.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include <utility>
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Cube::Cube() : GameObject("Cube") {
        addComponent<TechEngine::MeshRendererComponent>(new CubeMesh(),
                                                        &MaterialManager::getMaterial("DefaultMaterial"));

    }
}

