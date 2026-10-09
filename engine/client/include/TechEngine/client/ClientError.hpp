#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class ClientError : std::uint8_t { AlreadyStarted = 1, RendererStartFailed };

    const std::error_category& clientErrorCategory();

    std::error_code make_error_code(ClientError error);
}

template<>
struct std::is_error_code_enum<TechEngine::ClientError> : std::true_type {};
