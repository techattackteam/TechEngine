#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class ReadError : std::uint8_t {
        Truncated = 1,
        BadMagic,
        BadVersion,
    };

    const std::error_category& readErrorCategory();

    std::error_code make_error_code(ReadError error);
}

template<>
struct std::is_error_code_enum<TechEngine::ReadError> : std::true_type {};
