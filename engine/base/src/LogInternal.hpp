#pragma once

#include <TechEngine/base/Log.hpp>

#include <cstdint>
#include <string_view>

// Assert-only escape from the std::format seam. By the time Assert.cpp calls this, the
// message is already formatted (its own buffer) and the call site is trimmed file/function
// strings, not a std::source_location — so this hands the Logger a finished LogRecord instead
// of paying a second std::format pass over text that's already text. Private to base/src on
// purpose: nothing outside this module — and nothing but Assert.cpp inside it — should reach
// for this instead of TE_LOGGER_*.
namespace TechEngine::detail {
    void logRaw(Level level, LogChannel channel, std::string_view file, std::string_view function, std::uint32_t line, std::string_view message);
}
