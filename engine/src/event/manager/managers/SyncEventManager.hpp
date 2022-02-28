#include "../EventManager.hpp"

#pragma once
namespace Engine {
    class SyncEventManager : public EventManager {

    public:
        void execute() override;
    };
}

