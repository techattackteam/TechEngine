#pragma once

#include "event/manager/EventManager.hpp"

namespace TechEngine {
    class FixedSyncEventManager : public TechEngine::EventManager {
    public:
        void dispatch(TechEngine::Event *event) override;

        void execute() override;
    };
}
