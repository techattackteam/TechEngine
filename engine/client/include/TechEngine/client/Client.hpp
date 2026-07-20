#pragma once

namespace TechEngine {
    // Placeholder anchor for the `client` module (ADR-006 §1: presentation — rendering,
    // window, input, audio). Depends on core + platform + base. Linked by `runtime`
    // and `editor`, deliberately NOT by a future headless `runtime-server`.
    const char* clientVersion();
}
