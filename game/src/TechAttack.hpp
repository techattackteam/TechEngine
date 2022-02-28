#pragma once

#include <TechEngine.hpp>
#include <EntryPoint.hpp>

class TechAttack : public Engine::App {
public:
    Engine::StateMachine stateMachine{};

    TechAttack();

    void onUpdate();

    void keyPressedEvent(Engine::Event *event);

    void onFixedUpdate();
};


