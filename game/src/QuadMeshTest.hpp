#pragma once

#include <engine/scene/GameObject.hpp>
#include <sstream>

class QuadMeshTest : public Engine::GameObject {
public:
    QuadMeshTest(int name);

    std::string createName(int name);
};


