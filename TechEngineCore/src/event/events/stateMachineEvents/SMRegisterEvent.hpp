#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"

namespace TechEngineCore {
    class SMRegisterEvent : public Event {
    public:
        inline static EventType eventType = EventType("SMRegisterEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMRegisterEvent(StateMachine *stateMachine) : TechEngineCore::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMRegisterEvent() override = default;
    };
}

