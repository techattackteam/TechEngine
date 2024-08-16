#include "Client.hpp"

#include <iostream>

namespace TechEngine {
    void Client::init() {
        std::cout << "Client init" << std::endl;
    }

    void Client::onFixedUpdate() {
        std::cout << "Client onFixedUpdate" << std::endl;
    }

    void Client::onUpdate() {
        std::cout << "Client onUpdate" << std::endl;
    }

    void Client::destroy() {
        std::cout << "Client destroy" << std::endl;
    }
}
