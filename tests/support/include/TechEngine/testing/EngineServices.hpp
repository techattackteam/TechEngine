#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

namespace TechEngineTests {
    class EngineServices {
    public:
        TechEngine::MountTable mounts;
        TechEngine::FileAccess files;
        TechEngine::JobSystem jobs;
        TechEngine::Clock clock;
        TechEngine::EngineContext engine;
        TechEngine::InputFrame input;
        TechEngine::SimulationContext context;

        EngineServices() : files(mounts), jobs(1), engine{files, jobs, clock}, context{.fixedDeltaTime = 1.0 / 60.0, .tick = 1, .input = input, .engine = engine} {
        }
    };
}
