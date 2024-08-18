#pragma once

#include "scripting/Script.hpp"

class Test : public TechEngineAPI::Script {
    void onStart() override;
};

RegisterScript(Test)
