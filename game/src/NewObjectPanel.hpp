#pragma once

#include <Engine.hpp>
#include "QuadMeshTest.hpp"

class NewObjectPanel : Engine::CustomPanel {
public:
    int counter = 0;

    NewObjectPanel();

    void onUpdate() override;
};


