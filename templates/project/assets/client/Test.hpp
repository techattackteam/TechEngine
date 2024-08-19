#pragma once

#include "scripting/Script.hpp"

class Test : public TechEngineAPI::Script {
    void onStart() override;

    void onUpdate() override;
};

RegisterScript(Test)
