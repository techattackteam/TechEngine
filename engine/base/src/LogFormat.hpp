#pragma once

#include <TechEngine/base/Log.hpp>

#include "FormatBuffer.hpp"

namespace TechEngine {
    namespace detail {
        std::size_t flattenRecord(const LogRecord& record, char* out, std::size_t capacity);
    }
}