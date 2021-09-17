#include "QuadMeshTest.hpp"
#include <Engine.hpp>
#include <engine/components/TransformComponent.hpp>

QuadMeshTest::QuadMeshTest() : Engine::GameObject(true) {
    getComponent<Engine::TransformComponent>()->setPosition(glm::vec3(0, 0, -1));
    addComponent<Engine::MeshComponent>();
    getComponent<Engine::MeshComponent>()->createCube();
}
