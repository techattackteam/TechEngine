#include <jobs/DedicatedThreadState.hpp>

#include <mutex>

namespace TechEngine {
    void DedicatedThreadContext::State::finish(std::exception_ptr failure) {
        {
            std::lock_guard const lock{mutex};
            completion = ThreadCompletionResult{failure ? ThreadCompletionStatus::Failed : ThreadCompletionStatus::Completed, failure};
            if (!startupResolved) {
                startup = ThreadStartupResult{ThreadStartupStatus::Failed, failure};
                startupResolved = true;
                completion.status = ThreadCompletionStatus::Failed;
            }
        }
        changed.notify_all();
    }
}
