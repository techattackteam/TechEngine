#include <memory>
#include <utility>
#include "StateMachine.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/stateMachineEvents/SMRegisterEvent.hpp"
#include "event/events/stateMachineEvents/SMDeleteEvent.hpp"

namespace TechEngineCore {
    StateMachine::StateMachine() {
        states = std::unordered_map<StateName, State *>();
        EventDispatcher::getInstance().dispatch(new SMRegisterEvent(this));
    }

    StateMachine::~StateMachine() {
        EventDispatcher::getInstance().dispatch(new SMDeleteEvent(this));
    }


    void StateMachine::addState(State *state) {
        if (!states.contains(state->getName())) {
            states[state->getName()] = state;
            if (currentState == nullptr) {
                currentState = states[state->getName()];
            }
        }
    }

    void StateMachine::deleteState(const StateName &name) {
        if (states.contains(name)) {
            states.erase(name);
        }
    }


    bool StateMachine::hasState(const StateName &stateName) {
        return states.contains(stateName);
    }


    bool StateMachine::changeStates(const StateName &name) {
        if (hasState(name) && name != currentState->getName()) {
            nextState = name;
            changeState = true;
            return true;
        }
        return false;
    }

    void StateMachine::update() {
        if (changeState) {
            if (currentState != nullptr)
                currentState->leave();
            currentState = states[nextState];
            currentState->enter();
            changeState = false;
        }
    }

    bool StateMachine::addTransition(const StateName &from, const StateName &to) {
        if (states.contains(from)) {
            State *state = states[from];
            if (std::find(state->getTransitions().begin(), state->getTransitions().end(), to) == state->getTransitions().end()) {
                state->getTransitions().push_back(to);
                return true;
            }
        }
        return false;
    }

    bool StateMachine::removeTransition(const StateName &from, const StateName &to) {
        if (states.contains(from)) {
            State *state = states[from];
            auto index = std::find(state->getTransitions().begin(), state->getTransitions().end(), to);
            if (index != state->getTransitions().end()) {
                state->getTransitions().erase(index);
                return true;
            }
        }
        return false;
    }

    State *StateMachine::getCurrentState() {
        return currentState;
    }

}

