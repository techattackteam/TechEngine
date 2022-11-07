#pragma once

#include "event/manager/EventManager.hpp"

namespace TechEngine {
    class FixedSyncEventManager : public TechEngineCore::EventManager {
    public:
        void dispatch(TechEngineCore::Event *event) override;

        void execute() override;
    };
}
