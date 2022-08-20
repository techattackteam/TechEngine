
#include <condition_variable>
#include <thread>
#include <mutex>
#include "../EventManager.hpp"

#pragma once
namespace TechEngineCore {
    class AsyncEventManager : public EventManager {
    private:
        std::thread thread;
        std::condition_variable_any canExecuteCond;
        std::recursive_mutex mutex;

        bool running = false;
        bool canExecute = false;

        void join();

        void runThread();

    public:
        AsyncEventManager();

        ~AsyncEventManager();

        void dispatch(Event *event) override;
    };

}
