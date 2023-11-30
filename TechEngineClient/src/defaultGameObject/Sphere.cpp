#include "Sphere.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/SphereMesh.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Sphere::Sphere(Material *material) : GameObject("Sphere") {
        addComponent<TechEngine::MeshRendererComponent>(new TechEngine::SphereMesh(), material);
    }
}
