#pragma once

#include <iostream>
#include <events/input/KeyPressedEvent.hpp>

#include <script/Script.hpp>

#include "script/ScriptRegister.hpp"

class Test : public TechEngine::Script {
public:
    void onStart();

    void onKeyPressedEvent(TechEngine::KeyPressedEvent* event);
};

RegisterScript(Test)
