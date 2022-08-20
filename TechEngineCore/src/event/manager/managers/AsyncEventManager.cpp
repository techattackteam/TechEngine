#include <thread>
#include <iostream>
#include "AsyncEventManager.hpp"

namespace TechEngineCore {
    AsyncEventManager::AsyncEventManager() : EventManager() {
        thread = std::thread(&AsyncEventManager::runThread, this);
        running = true;
        canExecute = false;
    }

    AsyncEventManager::~AsyncEventManager() {
        running = false;
        join();
    }

    void AsyncEventManager::dispatch(Event *event) {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        EventManager::dispatch(event);
        canExecute = true;
        canExecuteCond.notify_one();
    }

    void AsyncEventManager::join() {
        thread.join();
    }

    void AsyncEventManager::runThread() {
        while (running) {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            while (!canExecute) {
                canExecuteCond.wait(lock);
            }
            execute();
            canExecute = false;
        }
    }

}