#pragma once

#include <vector>
#include "StateMachine.hpp"
#include "../event/events/stateMachineEvents/SMRegisterEvent.hpp"
#include "../event/events/stateMachineEvents/SMDeleteEvent.hpp"

namespace TechEngineCore {
    class StateMachineManager {
    private:
        std::vector<StateMachine *> stateMachines;

    public:
        inline static StateMachineManager *instance;

        static StateMachineManager &getInstance();

    public:
        StateMachineManager();

        ~StateMachineManager();

        void update();

        void registerSM(SMRegisterEvent *event);

        void removeSM(SMDeleteEvent *event);
    };
}

