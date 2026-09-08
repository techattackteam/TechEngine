#pragma once

#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <condition_variable>
#include <mutex>

namespace TechEngine {
    struct DedicatedThreadContext::State {
        std::mutex mutex;
        std::condition_variable changed;
        bool startupResolved = false;
        ThreadStartupResult startup;
        ThreadCompletionResult completion;

        void finish(std::exception_ptr failure);
    };
}
