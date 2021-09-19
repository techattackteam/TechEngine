#include "QuadMeshTest.hpp"
#include <Engine.hpp>
#include <engine/components/TransformComponent.hpp>
#include <sstream>

QuadMeshTest::QuadMeshTest(int name) : Engine::GameObject(createaName(name), false) {

    getComponent<Engine::TransformComponent>()->setPosition(glm::vec3(1, 0, -1));
    addComponent<Engine::MeshComponent>();
    getComponent<Engine::MeshComponent>()->createCube();
}

std::string QuadMeshTest::createaName(int name) {
    std::ostringstream oss;
    oss << "QuadMesh " << name;
    return oss.str();

}
