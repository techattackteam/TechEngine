#pragma once

#include <Engine.hpp>

class TestPanel2 : Engine::ImGuiPanel {
private:

    int counter;

    void onUpdate() override;

public:
    TestPanel2(std::string name);
};


