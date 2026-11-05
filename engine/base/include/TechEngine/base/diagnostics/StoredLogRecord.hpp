#pragma once

#include <TechEngine/base/diagnostics/Log.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    // Owns its message, so it outlives the log call. file and function stay views: they point
    // into source_location's static strings.
    struct StoredLogRecord {
        std::chrono::system_clock::time_point time;
        std::uint64_t tick = 0;
        Level level = Level::Info;
        LogModule moduleTag{};
        LogChannel channel{};
        std::string message;
        std::string_view file;
        std::string_view function;
        std::uint32_t line = 0;

        explicit StoredLogRecord(const LogRecord& record);
    };
}
