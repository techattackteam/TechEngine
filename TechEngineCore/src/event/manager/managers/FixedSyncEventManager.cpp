#include "FixedSyncEventManager.hpp"

namespace TechEngine {

    void FixedSyncEventManager::dispatch(TechEngineCore::Event *event) {
        EventManager::dispatch(event);
    }

    void FixedSyncEventManager::execute() {
        EventManager::execute();
    }
}
