#include "Cylinder.hpp"
#include "mesh/CylinderMesh.hpp"
#include "material/Material.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Cylinder::Cylinder(Material* material) : GameObject("Cylinder") {
        addComponent<MeshRendererComponent>(new CylinderMesh(), material);
    }
}
