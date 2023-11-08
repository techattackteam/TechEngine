#include "Sphere.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/SphereMesh.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    Sphere::Sphere() : GameObject("Sphere") {
        addComponent<TechEngine::MeshRendererComponent>(new TechEngine::SphereMesh(),
                                                        &MaterialManager::getMaterial("DefaultMaterial"));
    }
}
