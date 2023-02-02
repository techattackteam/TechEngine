#pragma once

#include <sstream>
#include "scene/GameObject.hpp"

class QuadMeshTest : public TechEngine::GameObject {
public:
    QuadMeshTest(std::string name);

    QuadMeshTest(std::string name, GameObject *gameObject);
};