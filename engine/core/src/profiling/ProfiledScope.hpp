#pragma once

#include "Profiler.hpp"
#include <chrono>

namespace TechEngine {
    class ProfiledScope {
    private:
        Profiler& m_profiler;
        std::string m_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;

    public:
        ProfiledScope(Profiler& profiler, const std::string& name)
            : m_profiler(profiler), m_name(name) {
            m_startTime = std::chrono::high_resolution_clock::now();
        }

        ~ProfiledScope() {
            auto endTime = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration<float, std::milli>(endTime - m_startTime).count();
            m_profiler.recordTiming(m_name, duration);
        }
    };
}
