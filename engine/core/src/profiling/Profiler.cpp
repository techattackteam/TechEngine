#include "Profiler.hpp"

namespace TechEngine {
    void Profiler::recordTiming(const std::string& systemName, float durationsMs) {
        auto& data = m_profilingData[systemName];
        data.systemName = systemName;
        data.updateTimeMs = durationsMs;
        const float alpha = 0.05f; // Lower alpha = smoother but slower to
        data.avgUpdateTimeMs = (data.updateTimeMs * alpha) + (data.avgUpdateTimeMs * (1.0f - alpha));
    }

    const std::unordered_map<std::string, SystemProfileData>& Profiler::getProfilingData() const {
        return m_profilingData;
    }
}
