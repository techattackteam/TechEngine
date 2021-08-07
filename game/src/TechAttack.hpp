#pragma once

#include <Engine.hpp>
#include <engine/core/EntryPoint.hpp>

class TechAttack : public Engine::App {
public:
    Engine::StateMachine stateMachine {};

    TechAttack();

    void onUpdate();
};


