#include <TechEngine/base/Log.hpp>

#include "FormatBuffer.hpp"
#include "SourceName.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <memory>
#include <vector>

namespace TechEngine {
    static constexpr std::size_t MESSAGE_CAPACITY = 1024;
    static constexpr std::uint16_t MAX_LOG_MODULES = 16;
    static constexpr std::uint16_t MAX_LOG_CHANNELS = 64;
    static constexpr std::size_t MAX_LOG_SINKS = 8;
    static constexpr std::size_t LOG_FILE_MAX_SIZE = std::size_t{5} * 1024 * 1024;
    static constexpr std::size_t LOG_FILE_MAX_COUNT = 3;
    static constexpr const char* LOG_FILE_PATH = "logs/techengine.log";

    struct LogModuleEntry {
        std::string_view name;
        std::atomic<Level> level{Level::Trace};
    };

    struct LogChannelEntry {
        std::string_view name;
        LogModule moduleTag{};
        std::atomic<Level> level{Level::Trace};
    };

    static std::atomic<Level> g_minLevel{Level::Trace};
    static std::atomic<std::uint64_t> g_frame{0};
    static std::atomic<bool> g_initialized{false};

    // Slot 0 is the default module/channel and is never handed out by registration.
    static std::array<LogModuleEntry, MAX_LOG_MODULES> g_modules{};
    static std::atomic<std::uint16_t> g_moduleCount{1};
    static std::array<LogChannelEntry, MAX_LOG_CHANNELS> g_channels{};
    static std::atomic<std::uint16_t> g_channelCount{1};

    // A null slot is an empty one — no separate count to keep in step with the array.
    static std::array<std::atomic<LogSinkFn>, MAX_LOG_SINKS> g_sinks{};

    static spdlog::level::level_enum toSpdlogLevel(Level level) {
        switch (level) {
            case Level::Trace:
                return spdlog::level::trace;
            case Level::Debug:
                return spdlog::level::debug;
            case Level::Info:
                return spdlog::level::info;
            case Level::Warn:
                return spdlog::level::warn;
            case Level::Error:
                return spdlog::level::err;
            case Level::Critical:
                return spdlog::level::critical;
            case Level::Off:
                return spdlog::level::off;
        }
        return spdlog::level::info;
    }

    static std::string_view levelTag(Level level) {
        switch (level) {
            case Level::Trace:
                return "TRACE";
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO";
            case Level::Warn:
                return "WARN";
            case Level::Error:
                return "ERROR";
            case Level::Critical:
                return "CRITICAL";
            case Level::Off:
                return "OFF";
        }
        return "INFO";
    }

    // std::localtime shares one static tm across threads; the CRT's reentrant spellings are
    // the portable fix. Not an OS header (ADR-005) — this is the C runtime.
    static std::tm localTime(std::time_t seconds) {
        std::tm out{};
#if defined(_WIN32)
        localtime_s(&out, &seconds);
#else
        localtime_r(&seconds, &out);
#endif
        return out;
    }

    static void spdlogSink(const LogRecord& record) {
        std::array<char, MESSAGE_CAPACITY + 256> storage;
        const std::size_t size = detail::flattenRecord(record, storage.data(), storage.size());

        spdlog::default_logger_raw()->log(toSpdlogLevel(record.level), spdlog::string_view_t{storage.data(), size});
    }

    bool addLogSink(LogSinkFn sink) {
        if (sink == nullptr) {
            return false;
        }
        for (std::atomic<LogSinkFn>& slot: g_sinks) {
            LogSinkFn expected = nullptr;
            if (slot.compare_exchange_strong(expected, sink, std::memory_order_acq_rel)) {
                return true;
            }
        }
        return false;
    }

    bool removeLogSink(LogSinkFn sink) {
        for (std::atomic<LogSinkFn>& slot: g_sinks) {
            LogSinkFn expected = sink;
            if (slot.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel)) {
                return true;
            }
        }
        return false;
    }

    static LogModuleEntry& moduleEntry(LogModule moduleTag) {
        const std::uint16_t id = moduleTag.id();
        return id < g_moduleCount.load(std::memory_order_acquire) ? g_modules[id] : g_modules[0];
    }

    static LogChannelEntry& channelEntry(LogChannel channel) {
        const std::uint16_t id = channel.id();
        return id < g_channelCount.load(std::memory_order_acquire) ? g_channels[id] : g_channels[0];
    }

    LogModule registerLogModule(std::string_view name, Level defaultLevel) {
        const std::uint16_t slot = g_moduleCount.fetch_add(1, std::memory_order_acq_rel);
        if (slot >= MAX_LOG_MODULES) {
            g_moduleCount.store(MAX_LOG_MODULES, std::memory_order_release);
            std::fprintf(stderr, "[log] module table full — '%.*s' falls back to default\n", static_cast<int>(name.size()), name.data());
            return DEFAULT_MODULE;
        }

        g_modules[slot].name = name;
        g_modules[slot].level.store(defaultLevel, std::memory_order_relaxed);
        return LogModule{slot};
    }

    LogChannel registerLogChannel(std::string_view name, LogModule moduleTag, Level defaultLevel) {
        const std::uint16_t slot = g_channelCount.fetch_add(1, std::memory_order_acq_rel);
        if (slot >= MAX_LOG_CHANNELS) {
            g_channelCount.store(MAX_LOG_CHANNELS, std::memory_order_release);
            std::fprintf(stderr, "[log] channel table full — '%.*s' falls back to default\n", static_cast<int>(name.size()), name.data());
            return DEFAULT_CHANNEL;
        }

        g_channels[slot].name = name;
        g_channels[slot].moduleTag = moduleTag;
        g_channels[slot].level.store(defaultLevel, std::memory_order_relaxed);
        return LogChannel{slot};
    }

    std::string_view logModuleName(LogModule moduleTag) {
        const std::string_view name = moduleEntry(moduleTag).name;
        return name.empty() ? std::string_view{"default"} : name;
    }

    std::string_view logChannelName(LogChannel channel) {
        const std::string_view name = channelEntry(channel).name;
        return name.empty() ? std::string_view{"default"} : name;
    }

    LogModule logChannelModule(LogChannel channel) {
        return channelEntry(channel).moduleTag;
    }

    void initLogging() {
        if (g_initialized.exchange(true, std::memory_order_acq_rel)) {
            return;
        }

        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(2);
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        try {
            sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LOG_FILE_PATH, LOG_FILE_MAX_SIZE, LOG_FILE_MAX_COUNT));
        } catch (const spdlog::spdlog_ex& e) {
            std::fprintf(stderr, "[log] file sink disabled (%s): %s\n", LOG_FILE_PATH, e.what());
        }

        auto logger = std::make_shared<spdlog::logger>("techengine", sinks.begin(), sinks.end());
        logger->set_pattern("%^%v%$");           // our line already carries the time and the level
        logger->set_level(spdlog::level::trace); // we filter; spdlog must not double-filter
        logger->flush_on(spdlog::level::err);
        spdlog::set_default_logger(std::move(logger));

        addLogSink(&spdlogSink);
    }

    void shutdownLogging() {
        if (!g_initialized.exchange(false, std::memory_order_acq_rel)) {
            return;
        }
        removeLogSink(&spdlogSink);
        spdlog::shutdown();
    }

    void flushLogs() {
        if (g_initialized.load(std::memory_order_acquire)) {
            spdlog::default_logger_raw()->flush();
        }
        std::fflush(stderr);
    }

    void setMinLevel(Level level) {
        g_minLevel.store(level, std::memory_order_relaxed);
    }

    Level minLevel() {
        return g_minLevel.load(std::memory_order_relaxed);
    }

    void setModuleLevel(LogModule moduleTag, Level level) {
        moduleEntry(moduleTag).level.store(level, std::memory_order_relaxed);
    }

    Level moduleLevel(LogModule moduleTag) {
        return moduleEntry(moduleTag).level.load(std::memory_order_relaxed);
    }

    void setChannelLevel(LogChannel channel, Level level) {
        channelEntry(channel).level.store(level, std::memory_order_relaxed);
    }

    Level channelLevel(LogChannel channel) {
        return channelEntry(channel).level.load(std::memory_order_relaxed);
    }

    bool isEnabled(Level level, LogChannel channel) {
        if (level == Level::Off) {
            return false;
        }

        const LogChannelEntry& entry = channelEntry(channel);
        const Level floor = std::max({g_minLevel.load(std::memory_order_relaxed), moduleEntry(entry.moduleTag).level.load(std::memory_order_relaxed), entry.level.load(std::memory_order_relaxed)});
        return level >= floor;
    }

    void setDiagnosticFrame(std::uint64_t frame) {
        g_frame.store(frame, std::memory_order_relaxed);
    }

    // No sink registered is not the same as a dropped record — before initLogging(), or after
    // shutdownLogging(), the line still has to reach somewhere (ADR-011 §2). That fallback is
    // what LogTests' captureStderr case pins.
    static void deliverRecord(const LogRecord& record) {
        bool delivered = false;
        for (const std::atomic<LogSinkFn>& slot: g_sinks) {
            const LogSinkFn sink = slot.load(std::memory_order_acquire);
            if (sink != nullptr) {
                sink(record);
                delivered = true;
            }
        }

        if (delivered) {
            return;
        }

        std::array<char, MESSAGE_CAPACITY + 256> line;
        const std::size_t size = detail::flattenRecord(record, line.data(), line.size());
        std::fprintf(stderr, "%.*s\n", static_cast<int>(size), line.data());
    }

    namespace detail {
        void logDispatch(Level level, LogChannel channel, const std::source_location& loc, std::string_view fmtStr, std::format_args args) {
            if (static_cast<int>(level) < TE_LOG_ACTIVE_LEVEL) {
                return;
            }

            if (!isEnabled(level, channel)) {
                return;
            }

            std::array<char, MESSAGE_CAPACITY> storage;
            FormatBuffer buffer{storage.data(), storage.size(), 0, false};

            try {
                std::vformat_to(FormatBufferIterator{buffer}, fmtStr, args);
            } catch (const std::exception& e) {
                buffer.size = 0;
                buffer.truncated = false;
                for (const char c: std::string_view{"<format error: "}) {
                    buffer.push(c);
                }
                for (const char c: std::string_view{e.what()}) {
                    buffer.push(c);
                }
                buffer.push('>');
            }

            buffer.markTruncated();

            const LogRecord record{
                .time = std::chrono::system_clock::now(),
                .frame = g_frame.load(std::memory_order_relaxed),
                .level = level,
                .moduleTag = logChannelModule(channel),
                .channel = channel,
                .message = std::string_view{buffer.data, buffer.size},
                .file = baseName(loc.file_name()),
                .function = shortFunctionName(loc.function_name()),
                .line = static_cast<std::uint32_t>(loc.line()), // line() is uint_least32_t
            };

            deliverRecord(record);
        }

        std::size_t flattenRecord(const LogRecord& record, char* out, std::size_t capacity) {
            FormatBuffer buffer{out, capacity, 0, false};

            const auto sinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(record.time.time_since_epoch());
            const std::tm local = localTime(std::chrono::system_clock::to_time_t(record.time));

            std::format_to(FormatBufferIterator{buffer},
                           "[{0:02}:{1:02}:{2:02}.{3:03}][f {4}][{5}/{6}][{7}:{8}:{9}()][{10}] "
                           "{11}",
                           local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(sinceEpoch.count() % 1000), record.frame, logModuleName(record.moduleTag), logChannelName(record.channel), record.file, record.line, record.function, levelTag(record.level), record.message);
            buffer.markTruncated();
            return buffer.size;
        }
    }
}