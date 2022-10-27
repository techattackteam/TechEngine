#pragma once

#include <sstream>
#include "scene/GameObject.hpp"

class QuadMeshTest : public TechEngine::GameObject {
public:
    QuadMeshTest(int name);

    std::string createName(int name);
};


