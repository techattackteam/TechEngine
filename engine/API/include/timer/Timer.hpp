#pragma once

#include "core/ExportDLL.hpp"

namespace TechEngine {
    class Timer;
}

namespace TechEngineAPI {
    class API_DLL Timer {
    private:
        friend class Entry;
        inline static TechEngine::Timer* timer;

        static void init(TechEngine::Timer* timer);

        static void shutdown();

    public:
        Timer() = delete;

        static double getDeltaTime();
    };
}
