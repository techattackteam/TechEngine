#pragma once

#include <TechEngine.hpp>
#include "QuadMeshTest.hpp"

class NewObjectPanel : Engine::CustomPanel {
public:
    int counter = 0;

    NewObjectPanel();

    void onUpdate() override;
};


