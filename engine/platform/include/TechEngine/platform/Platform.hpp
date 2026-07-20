#pragma once

namespace TechEngine {
    // Placeholder anchor for the `platform` module (ADR-006 §1: the OS seam — window,
    // input, file I/O, timer, dynamic-lib load). Depends on base.
    const char* platformVersion();
}
