#pragma once
#include "systems/System.hpp"
#include <glm/vec2.hpp>


namespace TechEngine {
    struct Viewport : public System {
        glm::vec2 position;
        glm::vec2 size;
        bool isFocused = false;
    };
}
