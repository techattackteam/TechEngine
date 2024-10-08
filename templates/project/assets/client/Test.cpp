#include "Test.hpp"

#include <common/include/logger/Logger.hpp>
#include <common/include/scene/Scene.hpp>
#include <common/include/components/Transform.hpp>
#include <common/include/components/MeshRenderer.hpp>
#include <common/include/resources/Resources.hpp>
#include <client/include/eventSystem/ClientEventSystem.hpp>
#include <client/include/events/Input/KeyPressEvent.hpp>

void Test::onStart() {
    LOGGER_INFO("Test onStart");
    // Plane vertices and indices
    std::vector<Vertex> planeVertices = {
        {{-0.5f, 0.0f, 0.5f}}, // top-left
        {{0.5f, 0.0f, 0.5f}}, // top-right
        {{0.5f, 0.0f, -0.5f}}, // bottom-right
        {{-0.5f, 0.0f, -0.5f}} // bottom-left
    };

    std::vector<int> planeIndices = {
        0, 1, 2, // first triangle
        0, 2, 3 // second triangle
    };
    test1 = Scene::createEntity("Test1");
    test2 = Scene::createEntity("Test2");
    Scene::getComponent<Transform>(test1)->setPosition({0, 0, 1});
    Scene::getComponent<Transform>(test2)->setPosition({0, 0, -1});
    std::shared_ptr<Mesh> mesh = Resources::createMesh("Test");
    mesh->setVertices(planeVertices, planeIndices);
    material = Resources::createMaterial("Test");
    Scene::addComponent<MeshRenderer>(test1, mesh, material);
    Scene::addComponent<MeshRenderer>(test2, mesh, material);
    startTime = std::chrono::high_resolution_clock::now();
    EventSystem::subscribe<KeyPressEvent>([this](const std::shared_ptr<Event>& event) {
        KeyPressEvent keyPressEvent = dynamic_cast<KeyPressEvent&>(*event);
        LOGGER_INFO("Key pressed: {0}", keyPressEvent.m_key.getKeyName());
    });
}

void Test::onUpdate() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    //Use sine wave to change the color of the cube
    material->setColor({0.5f + 0.5f * sin(time), 0.5f + 0.5f * sin(time + 2.09439510239f), 0.5f + 0.5f * sin(time + 4.18879020479f), 1.0f});
    Scene::getComponent<MeshRenderer>(test1)->paintMesh();
    Scene::getComponent<MeshRenderer>(test2)->paintMesh();

    //Use sine wave to move the cube
    Scene::getComponent<Transform>(test1)->setPosition({0, 0.5f * sin(time), 1});
    Scene::getComponent<Transform>(test2)->setPosition({0, 0.5f * sin(time), -1});
}
