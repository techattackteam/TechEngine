#include "QuadMeshTest.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include <sstream>
#include <utility>

QuadMeshTest::QuadMeshTest(std::string name) : TechEngine::GameObject(std::move(name)) {
    addComponent<TechEngine::MeshRendererComponent>(new TechEngine::CubeMesh(),
                                                    new Material(glm::vec4(1, 1, 1, 1), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 32.0f));

}

QuadMeshTest::QuadMeshTest(std::string name, TechEngine::GameObject *gameObject) : GameObject(name, gameObject) {

}
