#pragma once

#include <TechEngine.hpp>
#include <sstream>

class QuadMeshTest : public TechEngine::GameObject {
public:
    QuadMeshTest(int name);

    std::string createName(int name);
};


