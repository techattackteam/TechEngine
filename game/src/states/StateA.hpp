#pragma once

#include <Engine.hpp>

class StateA : public Engine::State {

public:
    inline static const Engine::StateName stateName = "StateA";

    StateA();

    void enter() override;

    void leave() override;


};


