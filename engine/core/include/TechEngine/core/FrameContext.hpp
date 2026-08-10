#pragma once

#include <TechEngine/core/EngineContext.hpp>

#include <cstdint>

namespace TechEngine {
    enum class Role : std::uint8_t { Client, ListenServer, DedicatedServer };

    // The reference member deletes copy-assignment: a frame is observed through the loop's
    // const&, never reseated.
    struct FrameContext {
        float deltaTime = 0.0F;
        float fixedDeltaTime = 0.0F;
        float alpha = 0.0F;
        std::uint64_t tick = 0;
        std::uint64_t frameIndex = 0;
        Role role = Role::Client;
        const EngineContext& engine;
    };
}
