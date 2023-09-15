#include "Sphere.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/SphereMesh.hpp"

namespace TechEngine {
    Sphere::Sphere() : GameObject("Sphere") {
        addComponent<TechEngine::MeshRendererComponent>(new TechEngine::SphereMesh(),
                                                        new Material(glm::vec4(1, 1, 1, 1),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     32.0f));
    }
}
