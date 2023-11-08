#pragma once

#include "scene/GameObject.hpp"
#include "mesh/SphereMesh.hpp"

namespace TechEngine::MaterialEditorGameObjects {
    class Sphere : public GameObject {
    private:
        SphereMesh mesh;
    public:
        Sphere();

        void init();
    };
}