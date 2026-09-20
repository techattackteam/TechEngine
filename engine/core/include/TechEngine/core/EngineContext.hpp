#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

namespace TechEngine {
    struct EngineContext {
        FileAccess& files;
        JobSystem& jobs;
        const Clock& clock;
        InputBuffer& input;
        Scene& scene;
        ComponentRegistry& registry;
        Schedule& schedule;
    };
}
