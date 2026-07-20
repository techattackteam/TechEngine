#pragma once

namespace TechEngine {
    // Engine entry point the leaf exes call. The real game loop and composition root
    // (ADR-006 §4) land here; this placeholder keeps main() thin from the first commit.
    // Returns a process exit code.
    int run();
}
