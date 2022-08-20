#pragma once

#include <TechEngine.hpp>
#include <EntryPoint.hpp>

class TechAttack : public TechEngine::App {
public:
    TechEngineCore::StateMachine stateMachine{};

    TechAttack();

    void onUpdate();

    void keyPressedEvent(TechEngineCore::Event *event);

    void onFixedUpdate();
};


