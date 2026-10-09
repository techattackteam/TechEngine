#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class ProjectError : std::uint8_t { ParseFailed = 1, SchemaInvalid };

    const std::error_category& projectErrorCategory();

    std::error_code make_error_code(ProjectError error);
}

template<>
struct std::is_error_code_enum<TechEngine::ProjectError> : std::true_type {};
