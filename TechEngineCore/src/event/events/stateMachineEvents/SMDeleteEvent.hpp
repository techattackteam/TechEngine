#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class /*Engine_API*/ SMDeleteEvent : public TechEngine::Event {
    public:
        inline static EventType eventType = EventType("SMDeleteEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMDeleteEvent(StateMachine *stateMachine) : TechEngine::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMDeleteEvent() override = default;
    };

}


