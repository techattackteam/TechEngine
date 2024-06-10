#pragma once

#include "script/Script.hpp"
#include "script/ScriptRegister.hpp"

class Test : public TechEngine::Script {
public:
    void onStart() override;
};

RegisterScript(Test);
