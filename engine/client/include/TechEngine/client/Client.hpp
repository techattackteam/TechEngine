#pragma once

namespace TechEngine {
    // Linked by `runtime` and `editor`, deliberately NOT by a future headless
    // `runtime-server`.
    const char* clientVersion();
}
