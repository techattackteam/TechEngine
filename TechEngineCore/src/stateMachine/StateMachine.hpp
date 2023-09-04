#pragma once


#include <unordered_map>
#include <memory>
#include "State.hpp"

#pragma once

namespace TechEngine {

    class StateMachine {
    private:
        State *currentState;
        StateName nextState;
        bool changeState = false;
    protected:
        std::unordered_map<StateName, State *> states;
    public:
        StateMachine();

        ~StateMachine();

        void addState(State *state);

        void deleteState(const StateName &name);

        bool hasState(const StateName &stateName);

        void update();

        bool changeStates(const StateName &name);

        bool addTransition(const StateName &from, const StateName &to);

        bool removeTransition(const StateName &from, const StateName &to);

        State *getCurrentState();

    };
}
