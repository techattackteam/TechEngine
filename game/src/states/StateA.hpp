#pragma once

#include <TechEngine.hpp>

class StateA : public TechEngineCore::State {

public:
    inline static const TechEngineCore::StateName stateName = "StateA";

    StateA();

    void enter() override;

    void leave() override;


};


