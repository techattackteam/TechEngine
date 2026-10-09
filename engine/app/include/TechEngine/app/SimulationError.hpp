#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class SimulationError : std::uint8_t { AlreadyRunning = 1, StartupFailed };

    const std::error_category& simulationErrorCategory();

    std::error_code make_error_code(SimulationError error);
}

template<>
struct std::is_error_code_enum<TechEngine::SimulationError> : std::true_type {};
