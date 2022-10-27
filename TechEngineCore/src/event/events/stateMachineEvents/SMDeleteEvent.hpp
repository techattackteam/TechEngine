#pragma once

#include "../Event.hpp"
#include "../../../stateMachine/StateMachine.hpp"
#include "core/Core.hpp"

namespace TechEngineCore {
    class /*Engine_API*/ SMDeleteEvent : public TechEngineCore::Event {
    public:
        inline static EventType eventType = EventType("SMDeleteEvent", ASYNC);
        StateMachine *stateMachine;

        explicit SMDeleteEvent(StateMachine *stateMachine) : TechEngineCore::Event(eventType) {
            this->stateMachine = stateMachine;
        }

        ~SMDeleteEvent() override = default;
    };

}


