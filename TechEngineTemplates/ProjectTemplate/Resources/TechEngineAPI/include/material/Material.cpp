#include "Material.hpp"

#include "material/Material.hpp"

namespace TechEngineAPI {
    Material::Material(TechEngine::Material* material) {
        this->material = material;
    }

    void Material::changeColor(glm::vec3 color) {
        material->getColor() = glm::vec4(color, 1.0f);
    }

    void Material::changeColor(glm::vec4 color) {
        material->getColor() = color;
    }
}
