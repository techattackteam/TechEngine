#include <engine/components/TransformComponent.hpp>
#include "GameObjectTest.hpp"

GameObjectTest::GameObjectTest() : Engine::GameObject("Camera", false) {
    addComponent<Engine::CameraComponent>(this, true);
    getComponent<Engine::TransformComponent>()->translateTo(glm::vec3(0, 0, 3));
    getComponent<Engine::CameraComponent>()->updateViewMatrix();
}
