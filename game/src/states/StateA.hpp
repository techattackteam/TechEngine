#pragma once

#include <TechEngine.hpp>

class StateA : public Engine::State {

public:
    inline static const Engine::StateName stateName = "StateA";

    StateA();

    void enter() override;

    void leave() override;


};


