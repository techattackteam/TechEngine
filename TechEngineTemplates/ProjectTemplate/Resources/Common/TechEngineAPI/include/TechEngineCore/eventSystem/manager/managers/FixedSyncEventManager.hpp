#pragma once

#include "eventSystem/manager/EventManager.hpp"

namespace TechEngine {
    class FixedSyncEventManager : public EventManager {
    public:
        void dispatch(TechEngine::Event *event) override;

        void execute() override;
    };
}
