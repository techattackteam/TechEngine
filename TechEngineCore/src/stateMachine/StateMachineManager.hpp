#pragma once

#include <vector>
#include "StateMachine.hpp"
#include "events/stateMachineEvents/SMRegisterEvent.hpp"
#include "events/stateMachineEvents/SMDeleteEvent.hpp"

namespace TechEngine {
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

