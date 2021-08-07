#include "StateB.hpp"

StateB::StateB() : State("StateB") {

}

void StateB::enter() {
    std::cout << "Entering B" << std::endl;
}

void StateB::leave() {
    std::cout << "Leaving B" << std::endl;
}
