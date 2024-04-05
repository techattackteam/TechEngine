#pragma once
#include "../EventManager.hpp"

namespace TechEngine {
    class SyncEventManager : public EventManager {
    public:
        void execute() override;

        SyncEventManager& operator=(const SyncEventManager& other);
    };
}
