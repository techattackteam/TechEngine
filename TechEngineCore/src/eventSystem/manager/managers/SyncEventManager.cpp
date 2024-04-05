#include "SyncEventManager.hpp"

namespace TechEngine {
    void SyncEventManager::execute() {
        EventManager::execute();
    }

    SyncEventManager& SyncEventManager::operator=(const SyncEventManager& other) {
        if (&other == this) {
            return *this;
        }
        observers = other.observers;
        return *this;
    }
}
