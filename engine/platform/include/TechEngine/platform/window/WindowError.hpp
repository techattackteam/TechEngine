#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class WindowError : std::uint8_t { AlreadyOpen = 1, InvalidSize, PlatformInitFailed, CreationFailed };

    const std::error_category& windowErrorCategory();

    std::error_code make_error_code(WindowError error);
}

template<>
struct std::is_error_code_enum<TechEngine::WindowError> : std::true_type {};
