#pragma once

#include <cstdint>
#include <format>
#include <source_location>
#include <string_view>

#define TE_LOG_LEVEL_TRACE 0
#define TE_LOG_LEVEL_DEBUG 1
#define TE_LOG_LEVEL_INFO 2
#define TE_LOG_LEVEL_WARN 3
#define TE_LOG_LEVEL_ERROR 4
#define TE_LOG_LEVEL_CRITICAL 5
#define TE_LOG_LEVEL_OFF 6

#if !defined(TE_LOG_ACTIVE_LEVEL)
#define TE_LOG_ACTIVE_LEVEL TE_LOG_LEVEL_TRACE
#endif

namespace TechEngine {
    enum class Level : std::uint8_t {
        Trace = TE_LOG_LEVEL_TRACE,
        Debug = TE_LOG_LEVEL_DEBUG,
        Info = TE_LOG_LEVEL_INFO,
        Warn = TE_LOG_LEVEL_WARN,
        Error = TE_LOG_LEVEL_ERROR,
        Critical = TE_LOG_LEVEL_CRITICAL,
        Off = TE_LOG_LEVEL_OFF,
    };

    // TODO(S2-T3): registerChannel + per-channel level array + module tags. ADR-011 §2.
    class LogChannel {
    public:
        constexpr LogChannel() = default;

        constexpr explicit LogChannel(std::uint16_t id) : m_id{id} {
        }

        [[nodiscard]] constexpr std::uint16_t id() const {
            return m_id;
        }

        [[nodiscard]] constexpr bool operator==(const LogChannel&) const = default;

    private:
        std::uint16_t m_id{0};
    };

    inline constexpr LogChannel DEFAULT_CHANNEL{};

    // `message` points into the dispatch call's stack buffer — a sink that outlives the call
    // must copy it. `file`/`function` are safe: they are substrings of source_location's
    // static strings.
    struct LogRecord {
        std::uint64_t frame{0};
        Level level{Level::Info};
        LogChannel channel{};
        std::string_view message;
        std::string_view file;
        std::string_view function;
        std::uint32_t line{0};
    };

    using LogSinkFn = void (*)(const LogRecord&);

    void initLogging();

    void shutdownLogging();

    void flushLogs();

    void setMinLevel(Level level);

    [[nodiscard]] Level minLevel();

    [[nodiscard]] bool isEnabled(Level level);

    LogSinkFn setLogSink(LogSinkFn sink);

    // ADR-011 §9. Consumed by S2-T7/T9.
    void setDiagnosticFrame(std::uint64_t frame);

    namespace detail {
        void logDispatch(Level level, const std::source_location& loc, std::string_view fmtStr,
                         std::format_args args);

        template<typename... Args>
        void logImpl(Level level, const std::source_location& loc,
                     std::format_string<Args...> fmtStr, Args&&... args) {
            logDispatch(level, loc, fmtStr.get(), std::make_format_args(args...));
        }
    }
}

// TE_LOG_PRIVATE_* are plumbing for the TE_LOGGER_* macros below. Call TE_LOGGER_<LEVEL>
// instead — these bypass the compile-time level gate, so a direct call ships Trace into a
// Release build. The preprocessor has no access control: the name and the CI guard
// (ci.yml → private-symbols) are the enforcement.
//
// The format string rides inside __VA_ARGS__ to avoid __VA_OPT__ — MSVC's traditional
// preprocessor lacks it without /Zc:preprocessor. Don't "fix" this into a named parameter.
#define TE_LOG_PRIVATE_EMIT(level, ...)                                                            \
    do {                                                                                           \
        ::TechEngine::detail::logImpl((level), ::std::source_location::current(), __VA_ARGS__);    \
    } while (0)

#define TE_LOG_PRIVATE_DISCARD(...)                                                                \
    do {                                                                                           \
    } while (0)

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_TRACE
#define TE_LOGGER_TRACE(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Trace, __VA_ARGS__)
#else
#define TE_LOGGER_TRACE(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_DEBUG
#define TE_LOGGER_DEBUG(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Debug, __VA_ARGS__)
#else
#define TE_LOGGER_DEBUG(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_INFO
#define TE_LOGGER_INFO(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Info, __VA_ARGS__)
#else
#define TE_LOGGER_INFO(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_WARN
#define TE_LOGGER_WARN(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Warn, __VA_ARGS__)
#else
#define TE_LOGGER_WARN(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_ERROR
#define TE_LOGGER_ERROR(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Error, __VA_ARGS__)
#else
#define TE_LOGGER_ERROR(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_CRITICAL
#define TE_LOGGER_CRITICAL(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Critical, __VA_ARGS__)
#else
#define TE_LOGGER_CRITICAL(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#endif