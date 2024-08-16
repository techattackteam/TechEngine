#include "Server.hpp"

#include <iostream>

namespace TechEngine {
    void Server::init() {
        std::cout << "Server init" << std::endl;
    }

    void Server::onFixedUpdate() {
        std::cout << "Server onFixedUpdate" << std::endl;
    }

    void Server::onUpdate() {
        //Server does nothing on update
    }

    void Server::destroy() {
        std::cout << "Server destroy" << std::endl;
    }
}
