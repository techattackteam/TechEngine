#pragma once

#include <cstdint>

namespace TechEngine {
    enum class Role { Client, ListenServer, DedicatedServer };

    // TODO(S3): + `const EngineContext& engine` (ADR-006 §4) — no engine services exist yet.
    struct FrameContext {
        float deltaTime{0.0F};
        float fixedDeltaTime{0.0F};
        float alpha{0.0F};
        std::uint64_t tick{0};
        std::uint64_t frameIndex{0};
        Role role{Role::Client};
    };
}
