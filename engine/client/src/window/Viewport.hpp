#pragma once
#include "systems/System.hpp"
#include <glm/vec2.hpp>

//TODO: Rethink if this class is needed
namespace TechEngine {
    struct CORE_DLL Viewport : public System {
        glm::vec2 position;
        glm::vec2 size;
        bool isFocused = false;
    };
}
