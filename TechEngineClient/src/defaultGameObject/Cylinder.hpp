#pragma once

#include "scene/GameObject.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class Cylinder : public GameObject {
    public:
        Cylinder(Material *material);
    };
}
