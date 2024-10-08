#pragma once

#include <common/include/entity/Entity.hpp>
#include <common/include/resources/Material.hpp>
#include <common/include/scripting/Script.hpp>

#include <chrono>

using namespace TechEngineAPI;

class Test : public Script {
    Entity test1 = 0;
    Entity test2 = 0;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    std::shared_ptr<Material> material;

    void onStart() override;

    void onUpdate() override;
};

RegisterScript(Test);
