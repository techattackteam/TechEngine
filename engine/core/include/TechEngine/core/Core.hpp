#pragma once

namespace TechEngine {
    // Placeholder anchor for the `core` module (ADR-006 §1: server-capable simulation
    // — ECS, events, resources, serialization, physics, job-system). Depends on
    // base + platform; deliberately cannot see `client` (the client/server seam).
    const char* coreVersion();
}
