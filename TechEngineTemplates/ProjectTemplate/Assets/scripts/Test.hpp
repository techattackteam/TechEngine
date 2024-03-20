#pragma once

#include <script/Script.hpp>
#include <script/ScriptEngine.hpp>
#include <events/input/KeyPressedEvent.hpp>
#include <events/input/KeyReleasedEvent.hpp>

class Test : public TechEngine::Script {
private:
    bool aKeyPressed = false;
    bool dKeyPressed = false;
    bool wKeyPressed = false;
    bool sKeyPressed = false;
public:
    void onStart() override;

    void onUpdate() override;

    void onFixedUpdate() override;

    void onKeyPressed(TechEngine::KeyPressedEvent& event);

    void onKeyReleased(TechEngine::KeyReleasedEvent& event);
};

RegisterScript(Test)
