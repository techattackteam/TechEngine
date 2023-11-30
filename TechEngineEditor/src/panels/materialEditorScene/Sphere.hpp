#pragma once

#include "scene/GameObject.hpp"
#include "mesh/SphereMesh.hpp"
#include "material/Material.hpp"

namespace TechEngine::MaterialEditorGameObjects {
    class Sphere : public GameObject {
    private:
        SphereMesh mesh;
    public:
        Sphere();

        void init(Material *material);
    };
}