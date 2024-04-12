#include "Sphere.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine::MaterialEditorGameObjects {
    Sphere::Sphere() : GameObject("Sphere Material Editor") {
        addComponent<TransformComponent>();
    }

    void Sphere::init(Material* material) {
        addComponent<MeshRendererComponent>(&mesh, material);
    }
}
