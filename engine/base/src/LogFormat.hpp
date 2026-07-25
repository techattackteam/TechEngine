#pragma once

#include <TechEngine/base/Log.hpp>

namespace TechEngine {
    namespace detail {
        inline constexpr std::string_view TRUNCATION_MARKER = "...[truncated]";

        std::size_t flattenRecord(const LogRecord& record, char* out, std::size_t capacity);
    }
}