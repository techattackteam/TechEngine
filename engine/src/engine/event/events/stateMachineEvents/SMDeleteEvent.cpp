#include "SMDeleteEvent.hpp"

namespace Engine {

    SMDeleteEvent::SMDeleteEvent(StateMachine *stateMachine) : Engine::Event(eventType) {
        this->stateMachine = stateMachine;
    }

    SMDeleteEvent::~SMDeleteEvent() {

    }
}
