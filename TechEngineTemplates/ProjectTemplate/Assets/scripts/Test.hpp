#pragma once

#include <scripts/ScriptEngineAPI.hpp>
#include <scripts/Script.hpp>

class Test : public TechEngine::Script {
public:
    void onStart() override;

    void onUpdate() override;

    void onFixedUpdate() override;
};

RegisterScript(Test)
