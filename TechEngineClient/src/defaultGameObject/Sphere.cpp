#include "Sphere.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "mesh/SphereMesh.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Sphere::Sphere(Material* material) : GameObject("Sphere") {
        addComponent<MeshRendererComponent>(new TechEngine::SphereMesh(), material);
    }
}
