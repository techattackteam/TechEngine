#include <TechEngine/base/diagnostics/Log.hpp>

#include <diagnostics/Diagnostics.hpp>

namespace TechEngine {
    DiagnosticsScope::DiagnosticsScope() {
        initLogging();
    }

    DiagnosticsScope::~DiagnosticsScope() {
        shutdownLogging();
    }
}
