#include "QuadMeshTest.hpp"
#include <Engine.hpp>
#include <engine/components/TransformComponent.hpp>

QuadMeshTest::QuadMeshTest() : Engine::GameObject("QuadMeshTest", true) {
    getComponent<Engine::TransformComponent>()->setPosition(glm::vec3(1, 0, -1));
    addComponent<Engine::MeshComponent>();
    getComponent<Engine::MeshComponent>()->createCube();
}
