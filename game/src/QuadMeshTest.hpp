#pragma once

#include <engine/scene/GameObject.hpp>
#include <sstream>

class QuadMeshTest : Engine::GameObject {
public:
    QuadMeshTest(int name);

    std::string createaName(int name);
};


