#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"

namespace Engine {
    class SMDeleteEvent : public Engine::Event {
    public:
        inline static EventType eventType = EventType("SMDeleteEvent", ASYNC);
        StateMachine *stateMachine;

        SMDeleteEvent(StateMachine *stateMachine);

        ~SMDeleteEvent() override;
    };
}


