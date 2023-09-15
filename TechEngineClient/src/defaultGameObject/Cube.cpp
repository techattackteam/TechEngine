#include "Cube.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include <utility>

Cube::Cube() : TechEngine::GameObject("Cube") {
    addComponent<TechEngine::MeshRendererComponent>(new TechEngine::CubeMesh(),
                                                    new Material(glm::vec4(1, 1, 1, 1),
                                                                 glm::vec3(1.0f, 1.0f, 1.0f),
                                                                 glm::vec3(1.0f, 1.0f, 1.0f),
                                                                 glm::vec3(1.0f, 1.0f, 1.0f),
                                                                 32.0f));

}

