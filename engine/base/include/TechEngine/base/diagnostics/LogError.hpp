#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class LogError : std::uint8_t { NullSink = 1, SinkTableFull, SinkNotRegistered };

    const std::error_category& logErrorCategory();

    std::error_code make_error_code(LogError error);
}

template<>
struct std::is_error_code_enum<TechEngine::LogError> : std::true_type {};
