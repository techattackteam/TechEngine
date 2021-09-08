#pragma once

#include <Engine.hpp>

class TestPanel1 : Engine::ImGuiPanel {
private:
    float f = 0.0f;

    void onUpdate() override;

public:
    TestPanel1(std::string name);
};


