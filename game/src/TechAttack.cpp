#include "TechAttack.hpp"
#include "states/StateA.hpp"
#include "states/StateB.hpp"
#include "world/World.hpp"
#include "panels/TestPanel1.hpp"
#include "panels/TestPanel2.hpp"

#include <Engine.hpp>
#include <memory>

TechAttack::TechAttack() {
    stateMachine.createState(StateA::stateName, std::make_shared<StateA>());
    stateMachine.createState(StateB::stateName, std::make_shared<StateB>());
    World world = World();
    world.test();
    new TestPanel1("test1");
    new TestPanel2("test2");
}

void TechAttack::onUpdate() {
    stateMachine.changeStates(StateB::stateName);
}

Engine::App *Engine::createApp() {
    return new TechAttack();
}

