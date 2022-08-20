#pragma once

#include <TechEngine.hpp>

class StateB : public TechEngineCore::State {
public:
    inline static const TechEngineCore::StateName stateName = "StateB";

    StateB();

    void enter() override;

    void leave() override;
};


