#include "QuadMeshTest.hpp"
#include <Engine.hpp>
#include <sstream>

QuadMeshTest::QuadMeshTest(int name) : Engine::GameObject(createaName(name), false) {
    addComponent<Engine::MeshComponent>();
    getComponent<Engine::MeshComponent>()->createCube();
}

std::string QuadMeshTest::createaName(int name) {
    std::ostringstream oss;
    oss << "QuadMesh " << name;
    return oss.str();

}
