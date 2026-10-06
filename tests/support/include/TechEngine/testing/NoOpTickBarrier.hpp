#pragma once

#include <TechEngine/core/systems/SerialExecutor.hpp>

#include <span>

namespace TechEngineTests {
    class NoOpTickBarrier final : public TechEngine::TickBarrierServices {
    public:
        void assignNetIds(TechEngine::Scene&, std::span<const TechEngine::Entity>) override {
        }
    };
}
