#include "TechAttack.hpp"
#include "states/StateA.hpp"
#include "states/StateB.hpp"
#include "Camera.hpp"
#include "QuadMeshTest.hpp"
#include "NewObjectPanel.hpp"
#include "Light.hpp"

#include <TechEngine.hpp>
#include <memory>
#include <iostream>

TechAttack::TechAttack() {
    stateMachine.createState(StateA::stateName, std::make_shared<StateA>());
    stateMachine.createState(StateB::stateName, std::make_shared<StateB>());
    new Camera();
    new QuadMeshTest(-1);
    new NewObjectPanel();
    new Light();
    new Light();
    new Light();


    Engine::EventDispatcher::getInstance().subscribe(Engine::KeyHoldEvent::eventType, [this](Engine::Event *event) {
        keyPressedEvent(event);
    });
}

void TechAttack::keyPressedEvent(Engine::Event *event) {
    std::cout << ((Engine::KeyPressedEvent *) event)->getKey().getKeCode() << std::endl;
}

void TechAttack::onUpdate() {
    stateMachine.changeStates(StateB::stateName);
}

void TechAttack::onFixedUpdate() {

}

Engine::App *Engine::createApp() {
    return new TechAttack();
}

