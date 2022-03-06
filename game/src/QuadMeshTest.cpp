#include "QuadMeshTest.hpp"
#include <sstream>

QuadMeshTest::QuadMeshTest(int name) : Engine::GameObject(createName(name), false) {
    addComponent<Engine::MeshRendererComponent>(this, new Engine::CubeMesh(),
                                                new Material(glm::vec4(1, 1, 1, 1), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 32.0f));

}

std::string QuadMeshTest::createName(int name) {
    std::ostringstream oss;
    oss << "QuadMesh " << name;
    return oss.str();
}
