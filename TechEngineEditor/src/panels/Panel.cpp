#include <iostream>
#include "Panel.hpp"

namespace TechEngine {
    Panel::Panel(const std::string &name) {
        this->name = name;
    }

    Panel::~Panel() {
    }
}
