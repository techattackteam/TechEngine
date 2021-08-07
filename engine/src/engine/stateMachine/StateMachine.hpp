#pragma once


#include <unordered_map>
#include <memory>
#include "State.hpp"

#pragma once

namespace Engine {

    class StateMachine {
    private:
        std::shared_ptr<State> currentState;
        StateName nextState;
        bool changeState = false;
    protected:
        std::unordered_map<StateName, std::shared_ptr<State>> states;
    public:
        StateMachine();

        ~StateMachine();

        void createState(const StateName &name, std::shared_ptr<State> state);

        void deleteState(const StateName &name);

        bool hasState(const StateName &stateName);

        void update();

        bool changeStates(const StateName &name);

        bool addTransition(const StateName &from, const StateName &to);

        bool removeTransition(const StateName &from, const StateName &to);

        std::shared_ptr<State> getCurrentState();

    };
}
