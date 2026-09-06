#pragma once

namespace TechEngine {
    class DiagnosticsScope {
    public:
        DiagnosticsScope();

        ~DiagnosticsScope();

        DiagnosticsScope(const DiagnosticsScope&) = delete;

        DiagnosticsScope& operator=(const DiagnosticsScope&) = delete;
    };
}
