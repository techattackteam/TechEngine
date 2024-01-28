#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"

namespace TechEngine {
    class SMRegisterEvent : public Event {
    public:
        inline static EventType eventType = EventType("SMRegisterEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMRegisterEvent(StateMachine *stateMachine) : TechEngine::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMRegisterEvent() override = default;
    };
}

