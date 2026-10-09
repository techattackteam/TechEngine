#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace TechEngine {
    enum class FileError : std::uint8_t { InvalidPath = 1, NoMount, NotFound, IsADirectory, NotADirectory, AlreadyExists, NotEmpty, AccessDenied, IoError };

    const std::error_category& fileErrorCategory();

    std::error_code make_error_code(FileError error);
}

template<>
struct std::is_error_code_enum<TechEngine::FileError> : std::true_type {};
