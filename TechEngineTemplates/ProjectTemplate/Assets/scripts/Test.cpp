#include <iostream>
#include "Test.hpp"

#include <scene/SceneManagerAPI.hpp>
#include <scene/GameObject.hpp>
#include <components/TransformComponent.hpp>
using namespace TechEngineAPI;

void Test::onStart() {
    auto scene = SceneManagerAPI::getScene();

    std::shared_ptr<GameObject> go = scene->createGameObject("TestObject");
    go->getTransform()->setPosition(glm::vec3(0, 1, 0));
}

void Test::onFixedUpdate() {
}

void Test::onUpdate() {
}
