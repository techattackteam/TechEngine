#include "../EventManager.hpp"

#pragma once
namespace TechEngine {
    class SyncEventManager : public EventManager {

    public:
        void execute() override;
    };
}

