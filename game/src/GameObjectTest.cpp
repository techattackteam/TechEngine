#include <engine/components/TransformComponent.hpp>
#include "GameObjectTest.hpp"

GameObjectTest::GameObjectTest() : Engine::GameObject("Camera", true) {
    addComponent<Engine::CameraComponent>(this, true);
    getComponent<Engine::TransformComponent>()->setPosition(glm::vec3(0, 0, 3));
    getComponent<Engine::CameraComponent>()->updateViewMatrix();
}
