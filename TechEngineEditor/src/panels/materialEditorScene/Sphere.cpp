#include "Sphere.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/SphereMesh.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine::MaterialEditorGameObjects {
    Sphere::Sphere() {
        addComponent<TransformComponent>();
    }

    void Sphere::init() {
        addComponent<MeshRendererComponent>(&mesh, &MaterialManager::getMaterial("DefaultMaterial"));
    }
}
