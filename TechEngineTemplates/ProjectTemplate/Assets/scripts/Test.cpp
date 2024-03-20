#include "Test.hpp"
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <ScriptingAPI/scene/SceneManagerAPI.hpp>
#include <ScriptingAPI/scene/GameObjectAPI.hpp>

#include <components/TransformComponent.hpp>
#include <components/MeshRendererComponent.hpp>
#include <ScriptingAPI/event/EventDispatcherAPI.hpp>
#include <ScriptingAPI/material/MaterialManagerAPI.hpp>
using namespace TechEngine;

void Test::onStart() {
    auto scene = SceneManagerAPI::getScene();

    std::shared_ptr<GameObjectAPI> go = scene->createGameObject("Test");
    auto transform = go->getTransform();
    transform->translate(glm::vec3(4, 0, 0));
    std::cout << "Position: " << transform->position.x << " " << transform->position.y << " " << transform->position.z << std::endl;
    go->addComponent<MeshRendererComponent>();
    auto* meshRenderer = go->getComponent<MeshRendererComponent>();
    Material& material = meshRenderer->getMaterial();
    material.setColor(glm::vec4(1, 0, 0, 1));

    EventDispatcherAPI::subscribe(KeyPressedEvent::eventType, [this](Event* event) {
        onKeyPressed((KeyPressedEvent&)*event);
    });
    EventDispatcherAPI::subscribe(KeyReleasedEvent::eventType, [this](Event* event) {
        onKeyReleased((KeyReleasedEvent&)*event);
    });
    std::string name = "TestMaterial";
    Material* material2 = MaterialManagerAPI::createMaterial(name);
    material2->setColor(glm::vec4(0, 1, 0, 1));
    meshRenderer->changeMaterial(*material2);
}


void Test::onFixedUpdate() {
    if(wKeyPressed) {
        auto scene = SceneManagerAPI::getScene();
        auto go = scene->getGameObject("Test");
        auto transform = go->getTransform();
        transform->translate(glm::vec3(0, 0, -0.1f));
    }
    if(sKeyPressed) {
        auto scene = SceneManagerAPI::getScene();
        auto go = scene->getGameObject("Test");
        auto transform = go->getTransform();
        transform->translate(glm::vec3(0, 0, 0.1f));
    }
    if(aKeyPressed) {
        auto scene = SceneManagerAPI::getScene();
        auto go = scene->getGameObject("Test");
        auto transform = go->getTransform();
        transform->translate(glm::vec3(-0.1f, 0, 0));
    }
    if(dKeyPressed) {
        auto scene = SceneManagerAPI::getScene();
        auto go = scene->getGameObject("Test");
        auto transform = go->getTransform();
        transform->translate(glm::vec3(0.1f, 0, 0));
    }
}

void Test::onUpdate() {
}

void Test::onKeyPressed(KeyPressedEvent& event) {
    if (event.getKey().getKeyCode() == W) {
        wKeyPressed = true;
    } else if (event.getKey().getKeyCode() == S) {
        sKeyPressed = true;
    } else if (event.getKey().getKeyCode() == A) {
        aKeyPressed = true;
    } else if (event.getKey().getKeyCode() == D) {
        dKeyPressed = true;
    }

}

void Test::onKeyReleased(TechEngine::KeyReleasedEvent& event) {
    if (event.getKey().getKeyCode() == W) {
        wKeyPressed = false;
    } else if (event.getKey().getKeyCode() == S) {
        sKeyPressed = false;
    } else if (event.getKey().getKeyCode() == A) {
        aKeyPressed = false;
    } else if (event.getKey().getKeyCode() == D) {
        dKeyPressed = false;
    }
}
