#pragma once

#include <TechEngine.hpp>

class StateB : public Engine::State {
public:
    inline static const Engine::StateName stateName = "StateB";

    StateB();

    void enter() override;

    void leave() override;
};


