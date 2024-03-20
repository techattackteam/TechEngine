#include <thread>
#include <iostream>
#include "AsyncEventManager.hpp"

namespace TechEngine {
    AsyncEventManager::AsyncEventManager() : EventManager() {
        thread = new std::thread(&AsyncEventManager::runThread, this);
        running = true;
        canExecute = false;
    }

    AsyncEventManager::~AsyncEventManager() {
        running = false;
        canExecute = true;
        canExecuteCond.notify_all();
        thread->join();
        delete thread;
    }

    AsyncEventManager& AsyncEventManager::operator=(const AsyncEventManager& asyncEventManager) {
        if (this == &asyncEventManager) {
            return *this;
        }
        this->running = false;
        this->canExecute = true;
        this->canExecuteCond.notify_all();
        this->thread->join();
        delete this->thread;
        this->thread = new std::thread(&AsyncEventManager::runThread, this);
        this->running = true;
        this->canExecute = false;
        for (auto& observer : asyncEventManager.observers) {
            for (auto& callback : observer.second) {
                this->subscribe(observer.first, callback);
            }
        }
        this->dispatchedEvents = asyncEventManager.dispatchedEvents;
        return *this;
    }

    void AsyncEventManager::dispatch(Event* event) {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        EventManager::dispatch(event);
        canExecute = true;
        canExecuteCond.notify_one();
    }

    void AsyncEventManager::join() {
        thread->join();
    }

    void AsyncEventManager::runThread() {
        while (running) {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            while (!canExecute) {
                canExecuteCond.wait(lock);
            }
            //if (!running) Not sure why this is here
            execute();
            canExecute = false;
        }
    }
}
