#include "TechAttack.hpp"
#include "states/StateA.hpp"
#include "states/StateB.hpp"
#include "Camera.hpp"
#include "QuadMeshTest.hpp"
#include "NewObjectPanel.hpp"

#include <Engine.hpp>
#include <memory>

TechAttack::TechAttack() {
    stateMachine.createState(StateA::stateName, std::make_shared<StateA>());
    stateMachine.createState(StateB::stateName, std::make_shared<StateB>());
    new Camera();
    new QuadMeshTest(-1);
    new NewObjectPanel();

    Engine::subscribeEvent(Engine::KeyHoldEvent::eventType, [this](Engine::Event *event) {
        keyPressedEvent(event);
    });
}

void TechAttack::keyPressedEvent(Engine::Event *event) {
    std::cout << ((Engine::KeyPressedEvent *) event)->getKey().getKeCode() << std::endl;
}

void TechAttack::onUpdate() {
    stateMachine.changeStates(StateB::stateName);
}

Engine::App *Engine::createApp() {
    return new TechAttack();
}

