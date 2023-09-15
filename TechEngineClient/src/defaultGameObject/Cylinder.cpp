#include "Cylinder.hpp"
#include "mesh/CylinderMesh.hpp"
#include "mesh/Material.hpp"
#include "components/MeshRendererComponent.hpp"

namespace TechEngine {
    Cylinder::Cylinder() : GameObject("Cylinder") {
        addComponent<TechEngine::MeshRendererComponent>(new CylinderMesh(),
                                                        new Material(glm::vec4(1, 1, 1, 1),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     glm::vec3(1.0f, 1.0f, 1.0f),
                                                                     32.0f));
    }
}


