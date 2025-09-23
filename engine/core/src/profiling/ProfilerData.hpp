#pragma once

#include <string>

namespace TechEngine {
    struct SystemProfileData {
        std::string systemName;

        // Per-frame timings
        float updateTimeMs = 0.0f;
        float fixedUpdateTimeMs = 0.0f;

        // Smoothed timings for better readability
        float avgUpdateTimeMs = 0.0f;
        float avgFixedUpdateTimeMs = 0.0f;
    };
}
