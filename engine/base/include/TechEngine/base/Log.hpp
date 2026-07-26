#pragma once

#include <chrono>
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

    class LogModule {
    public:
        constexpr LogModule() = default;

        constexpr explicit LogModule(std::uint16_t id) : m_id{id} {
        }

        constexpr std::uint16_t id() const {
            return m_id;
        }

        constexpr bool operator==(const LogModule&) const = default;

    private:
        std::uint16_t m_id{0};
    };

    class LogChannel {
    public:
        constexpr LogChannel() = default;

        constexpr explicit LogChannel(std::uint16_t id) : m_id{id} {
        }

        constexpr std::uint16_t id() const {
            return m_id;
        }

        constexpr bool operator==(const LogChannel&) const = default;

    private:
        std::uint16_t m_id{0};
    };

    inline constexpr LogModule DEFAULT_MODULE{};
    inline constexpr LogChannel DEFAULT_CHANNEL{};

    // `message` points into the dispatch call's stack buffer — a sink that outlives the call
    // must copy it. `file`/`function` are safe: they are substrings of source_location's
    // static strings.
    struct LogRecord {
        std::chrono::system_clock::time_point time;
        std::uint64_t frame{0};
        Level level{Level::Info};
        LogModule moduleTag{};
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

    // `name` is stored, never copied — pass a literal or a static. ADR-011 §2.
    LogModule registerLogModule(std::string_view name, Level defaultLevel = Level::Trace);

    LogChannel registerLogChannel(std::string_view name, LogModule moduleTag, Level defaultLevel = Level::Trace);

    std::string_view logModuleName(LogModule moduleTag);

    std::string_view logChannelName(LogChannel channel);

    LogModule logChannelModule(LogChannel channel);

    void setMinLevel(Level level);

    Level minLevel();

    void setModuleLevel(LogModule moduleTag, Level level);

    Level moduleLevel(LogModule moduleTag);

    void setChannelLevel(LogChannel channel, Level level);

    Level channelLevel(LogChannel channel);

    bool isEnabled(Level level, LogChannel channel = DEFAULT_CHANNEL);

    // Sinks are added, never swapped: a test that replaced the set would delete the real
    // path and pass against nothing (S2-T2). Returns false when the set is full.
    bool addLogSink(LogSinkFn sink);

    bool removeLogSink(LogSinkFn sink);

    // ADR-011 §9. Consumed by S2-T7/T9.
    void setDiagnosticFrame(std::uint64_t frame);

    namespace detail {
        void logDispatch(Level level, LogChannel channel, const std::source_location& loc, std::string_view fmtStr, std::format_args args);

        template<typename... Args>
        void logImpl(Level level, LogChannel channel, const std::source_location& loc, std::format_string<Args...> fmtStr, Args&&... args) {
            logDispatch(level, channel, loc, fmtStr.get(), std::make_format_args(args...));
        }
    }
}

// GOTCHA: define TE_LOG_CHANNEL *before* including this header, or the fallback below wins
// and your later #define is a redefinition. One channel per TU is the intended shape; reach
// for TE_LOGGER_<LEVEL>_CH only for the cross-cutting exception.
#if !defined(TE_LOG_CHANNEL)
#define TE_LOG_CHANNEL ::TechEngine::DEFAULT_CHANNEL
#endif

// TE_LOG_PRIVATE_* are plumbing for the TE_LOGGER_* macros below. Call TE_LOGGER_<LEVEL>
// instead — these bypass the compile-time level gate, so a direct call ships Trace into a
// Release build. The preprocessor has no access control: the name and the CI guard
// (ci.yml → private-symbols) are the enforcement.
//
// The format string rides inside __VA_ARGS__ to avoid __VA_OPT__ — MSVC's traditional
// preprocessor lacks it without /Zc:preprocessor. Don't "fix" this into a named parameter.
#define TE_LOG_PRIVATE_EMIT(level, channel, ...)                                                                                                                                                                                                                                                                                                                                           \
    do {                                                                                                                                                                                                                                                                                                                                                                                   \
        ::TechEngine::detail::logImpl((level), (channel), ::std::source_location::current(), __VA_ARGS__);                                                                                                                                                                                                                                                                                 \
    } while (0)

#define TE_LOG_PRIVATE_DISCARD(...)                                                                                                                                                                                                                                                                                                                                                        \
    do {                                                                                                                                                                                                                                                                                                                                                                                   \
    } while (0)

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_TRACE
#define TE_LOGGER_TRACE(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Trace, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_TRACE_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Trace, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_TRACE(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_TRACE_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_DEBUG
#define TE_LOGGER_DEBUG(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Debug, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_DEBUG_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Debug, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_DEBUG(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_DEBUG_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_INFO
#define TE_LOGGER_INFO(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Info, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_INFO_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Info, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_INFO(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_INFO_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_WARN
#define TE_LOGGER_WARN(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Warn, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_WARN_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Warn, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_WARN(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_WARN_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_ERROR
#define TE_LOGGER_ERROR(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Error, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_ERROR_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Error, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_ERROR(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_ERROR_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_CRITICAL
#define TE_LOGGER_CRITICAL(...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Critical, TE_LOG_CHANNEL, __VA_ARGS__)
#define TE_LOGGER_CRITICAL_CH(channel, ...) TE_LOG_PRIVATE_EMIT(::TechEngine::Level::Critical, (channel), __VA_ARGS__)
#else
#define TE_LOGGER_CRITICAL(...) TE_LOG_PRIVATE_DISCARD(__VA_ARGS__)
#define TE_LOGGER_CRITICAL_CH(channel, ...) TE_LOG_PRIVATE_DISCARD((channel), __VA_ARGS__)
#endif