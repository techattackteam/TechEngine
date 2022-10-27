#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"
#include "core/Core.hpp"

namespace TechEngineCore {
    class /*Engine_API*/ SMRegisterEvent : public Event {
    public:
        inline static EventType eventType = EventType("SMRegisterEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMRegisterEvent(StateMachine *stateMachine) : TechEngineCore::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMRegisterEvent() override = default;
    };
}

