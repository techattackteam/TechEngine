#pragma once

#include <TechEngine.hpp>
#include "QuadMeshTest.hpp"

class NewObjectPanel : TechEngine::CustomPanel {
public:
    int counter = 0;

    NewObjectPanel();

    void onUpdate() override;
};


