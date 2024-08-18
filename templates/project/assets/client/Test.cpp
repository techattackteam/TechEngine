#include "Test.hpp"

#include <iostream>
#include "timer/Timer.hpp"

void Test::onStart() {
    std::cout << "Hello from Test script!" << std::endl;
    std::cout << std::to_string(TechEngineAPI::Timer::getDeltaTime()) << std::endl;
}
