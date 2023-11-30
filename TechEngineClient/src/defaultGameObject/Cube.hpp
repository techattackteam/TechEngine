#pragma once

#include <sstream>
#include "scene/GameObject.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class Cube : public GameObject {
    public:
        Cube(Material *material);
    };
}
