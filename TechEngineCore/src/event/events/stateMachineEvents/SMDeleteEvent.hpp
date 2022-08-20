#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"

namespace TechEngineCore {
    class SMDeleteEvent : public TechEngineCore::Event {
    public:
        inline static EventType eventType = EventType("SMDeleteEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMDeleteEvent(StateMachine *stateMachine) : TechEngineCore::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMDeleteEvent() override = default;
    };

}


