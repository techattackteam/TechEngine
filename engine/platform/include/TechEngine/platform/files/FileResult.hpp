#pragma once

#include <cstdint>

namespace TechEngine {
    enum class FileResult : std::uint8_t { Ok, InvalidPath, NoMount, NotFound, NotADirectory, AccessDenied, IoError };
}
