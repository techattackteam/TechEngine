#pragma once

#include "scene/GameObject.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class Sphere : public GameObject {
    public:
        Sphere(Material *material);
    };
}
