#include "Cylinder.hpp"
#include "mesh/CylinderMesh.hpp"
#include "material/Material.hpp"
#include "components/MeshRendererComponent.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Cylinder::Cylinder() : GameObject("Cylinder") {
        addComponent<TechEngine::MeshRendererComponent>(new CylinderMesh(),
                                                        &MaterialManager::getMaterial("DefaultMaterial"));
    }
}


