#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <cstdint>

namespace TechEngine {
    enum class Role : std::uint8_t { Client, ListenServer, DedicatedServer };

    struct SimulationContext {
        double fixedDeltaTime = 0.0;
        std::uint64_t tick = 0;
        std::uint64_t timeline = 0;
        Clock::TimePoint tickTime{};
        Role role = Role::Client;
        const InputFrame& input;
        const EngineContext& engine;
    };
}
