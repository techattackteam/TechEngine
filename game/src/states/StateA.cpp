#include "StateA.hpp"


StateA::StateA() : State(stateName) {

}


void StateA::enter() {
    std::cout << "Entering State A" << std::endl;
}

void StateA::leave() {
    std::cout << "Leaving State A" << std::endl;

}
