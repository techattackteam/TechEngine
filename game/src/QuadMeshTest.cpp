#include "QuadMeshTest.hpp"
#include <sstream>

QuadMeshTest::QuadMeshTest(int name) : Engine::GameObject(createName(name), false) {
    addComponent<Engine::MeshRendererComponent>(this, new Engine::CubeMesh());

}

std::string QuadMeshTest::createName(int name) {
    std::ostringstream oss;
    oss << "QuadMesh " << name;
    return oss.str();
}
