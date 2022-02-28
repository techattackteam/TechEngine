#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"

namespace Engine {
    class SMRegisterEvent : public Engine::Event {
    public:
        inline static EventType eventType = EventType("SMRegisterEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMRegisterEvent(StateMachine *stateMachine) : Engine::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMRegisterEvent() override = default;
    };
}

