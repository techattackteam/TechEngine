#include "SMRegisterEvent.hpp"

namespace Engine {
    Engine::SMRegisterEvent::SMRegisterEvent(StateMachine *stateMachine) : Engine::Event(eventType) {
        this->stateMachine = stateMachine;
    }

    SMRegisterEvent::~SMRegisterEvent() = default;
}

