#include "../EventManager.hpp"

#pragma once
namespace TechEngineCore {
    class SyncEventManager : public EventManager {

    public:
        void execute() override;
    };
}

