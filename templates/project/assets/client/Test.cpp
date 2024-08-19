#include "Test.hpp"

#include <iostream>

void Test::onStart() {
    std::cout << "Hello from Test script!" << std::endl;
}

void Test::onUpdate() {
    std::cout << "Update from Test script!" << std::endl;
}
