#pragma once

#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

namespace TechEngine {
    struct EngineContext {
        FileAccess& files;
        JobSystem& jobs;
    };
}
