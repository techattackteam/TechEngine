#include "FixedSyncEventManager.hpp"

namespace TechEngine {

    void FixedSyncEventManager::dispatch(TechEngine::Event *event) {
        EventManager::dispatch(event);
    }

    void FixedSyncEventManager::execute() {
        EventManager::execute();
    }
}
