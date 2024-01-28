#pragma once

#include "C:/dev/TechEngine/bin/TechEngineEditor/debug/TechEngineAPI/include/TechEngineGame/script/Script.hpp"

class Test : public TechEngine::Script {
public:
    void onUpdate() override;

    void onFixedUpdate() override;

};


