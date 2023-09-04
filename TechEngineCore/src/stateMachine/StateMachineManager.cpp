#include "StateMachineManager.hpp"
#include "../event/EventDispatcher.hpp"

namespace TechEngine {
    StateMachineManager::StateMachineManager() {
        instance = this;

        EventDispatcher::getInstance().subscribe(SMRegisterEvent::eventType, [this](Event *event) {
            registerSM((SMRegisterEvent * )(event));
        });
        EventDispatcher::getInstance().subscribe(SMDeleteEvent::eventType, [this](Event *event) {
            removeSM((SMDeleteEvent * )(event));
        });
    }

    StateMachineManager::~StateMachineManager() {

    }

    void StateMachineManager::update() {
        for (StateMachine *stateMachine: stateMachines) {
            stateMachine->update();
        }
    }

    void StateMachineManager::registerSM(SMRegisterEvent *event) {
        stateMachines.emplace_back(event->stateMachine);
    }

    void StateMachineManager::removeSM(SMDeleteEvent *event) {
        auto index = std::find(stateMachines.begin(), stateMachines.end(), event->stateMachine);
        if (index != stateMachines.end()) {
            stateMachines.erase(index);
        }
    }

    StateMachineManager &StateMachineManager::getInstance() {
        return *instance;
    }

}
