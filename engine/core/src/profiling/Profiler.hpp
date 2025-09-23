#pragma once

#include "ProfilerData.hpp"
#include "systems/System.hpp"

#include <unordered_map>

namespace TechEngine {
    class CORE_DLL Profiler : public System {
    private:
        std::unordered_map<std::string, SystemProfileData> m_profilingData;

    public:
        void recordTiming(const std::string& systemName, float durationsMs);

        const std::unordered_map<std::string, SystemProfileData>& getProfilingData() const;
    };
}
