#include "TechAttack.hpp"
#include "states/StateA.hpp"
#include "states/StateB.hpp"
#include "Camera.hpp"
#include "QuadMeshTest.hpp"
#include "NewObjectPanel.hpp"
#include "NetworkHandler.hpp"

#include <memory>
#include <iostream>

TechAttack::TechAttack() {
    stateMachine.createState(StateA::stateName, std::make_shared<StateA>());
    stateMachine.createState(StateB::stateName, std::make_shared<StateB>());
    new Camera();
    new QuadMeshTest(-1);
    new NewObjectPanel();
    new NetworkHandler();

    TechEngine::subscribeEvent(TechEngine::KeyPressedEvent::eventType, [this](TechEngineCore::Event *event) {
        keyPressedEvent(event);
    });
}

void TechAttack::keyPressedEvent(TechEngineCore::Event *event) {
    std::cout << ((TechEngine::KeyPressedEvent *) event)->getKey().getKeyName() << std::endl;
}

void TechAttack::onUpdate() {
    stateMachine.changeStates(StateB::stateName);
}

void TechAttack::onFixedUpdate() {

}

TechEngineCore::App *TechEngineCore::createApp() {
    return new TechAttack();
}

