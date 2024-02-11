#pragma once
#include <condition_variable>
#include <thread>
#include <mutex>
#include "../EventManager.hpp"

namespace TechEngine {
    class AsyncEventManager : public EventManager {
    private:
        std::thread *thread;
        std::condition_variable_any canExecuteCond;
        std::recursive_mutex mutex;

        std::atomic_bool running = false;
        std::atomic_bool canExecute = false;

        void join();

        void runThread();

    public:
        AsyncEventManager();

        ~AsyncEventManager();

        void dispatch(Event *event) override;
    };
}
