#pragma once

namespace TechEngine {
    // GOTCHA: the state behind this is process-global, not scope-local. A second scope
    // constructed while one is alive initializes nothing, and its destructor shuts logging
    // down for both. One per process, at the composition root.
    class DiagnosticsScope {
    public:
        DiagnosticsScope();

        ~DiagnosticsScope();

        DiagnosticsScope(const DiagnosticsScope&) = delete;

        DiagnosticsScope& operator=(const DiagnosticsScope&) = delete;
    };
}
