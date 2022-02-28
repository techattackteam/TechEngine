#include "State.hpp"

namespace Engine {
    State::State(const StateName &stateName) {
        this->stateName = stateName;
    }

    StateName State::getStateName() {
        return stateName;
    }

    std::vector<StateName> State::getTransitions() {
        return transitions;
    }
}
