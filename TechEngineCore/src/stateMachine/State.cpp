#include "State.hpp"

namespace TechEngineCore {
    State::State(const StateName &stateName) {
        this->stateName = stateName;
    }

    StateName State::getName() {
        return stateName;
    }

    std::vector<StateName> State::getTransitions() {
        return transitions;
    }
}
