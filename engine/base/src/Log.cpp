#include <TechEngine/base/Log.hpp>

// The ONLY TU that may include spdlog — ADR-011 §1.
#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <cstdio>
#include <iterator>

namespace TechEngine {
    static constexpr std::size_t MESSAGE_CAPACITY = 1024;
    static constexpr std::string_view TRUNCATION_MARKER = "…[truncated]";

    struct LogFormatBuffer {
        char* data{nullptr};
        std::size_t capacity{0};
        std::size_t size{0};
        bool truncated{false};

        void push(char c) {
            if (size < capacity) {
                data[size++] = c;
            } else {
                truncated = true;
            }
        }
    };

    class LogFormatBufferIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = char;

        LogFormatBufferIterator() = default;

        explicit LogFormatBufferIterator(LogFormatBuffer& buffer) : m_buffer{&buffer} {
        }

        // const because that is what std::indirectly_writable requires; drop it and the
        // static_assert below fires.
        const LogFormatBufferIterator& operator*() const {
            return *this;
        }

        const LogFormatBufferIterator& operator=(char c) const {
            m_buffer->push(c);
            return *this;
        }

        LogFormatBufferIterator& operator++() {
            return *this;
        }

        LogFormatBufferIterator operator++(int) {
            return *this;
        }

    private:
        LogFormatBuffer* m_buffer{nullptr};
    };

    static_assert(std::output_iterator<LogFormatBufferIterator, char>,
                  "vformat_to needs this to model output_iterator");

    static std::atomic<Level> g_minLevel{Level::Trace};
    static std::atomic<std::uint64_t> g_frame{0};
    static std::atomic<bool> g_initialized{false};

    static void spdlogSink(const LogRecord& record);

    static std::atomic<LogSinkFn> g_sink{&spdlogSink};

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

    static void flattenRecord(const LogRecord& record, LogFormatBuffer& out) {
        // TODO(S2-T3): prepend channel + module tag. ADR-011 §3.
        std::format_to(LogFormatBufferIterator{out}, "[f {0}][{1}:{2}:{3}()] {4}", record.frame,
                       record.file, record.line, record.function, record.message);
    }

    // TODO(S2-T3): replace with the real sink set. ADR-011 §3.
    static void spdlogSink(const LogRecord& record) {
        if (!g_initialized.load(std::memory_order_acquire)) {
            std::fprintf(stderr, "[f %llu][%.*s:%u:%.*s()] %.*s\n",
                         static_cast<unsigned long long>(record.frame),
                         static_cast<int>(record.file.size()), record.file.data(), record.line,
                         static_cast<int>(record.function.size()), record.function.data(),
                         static_cast<int>(record.message.size()), record.message.data());
            return;
        }

        std::array<char, MESSAGE_CAPACITY + 256> storage{};
        LogFormatBuffer line{storage.data(), storage.size(), 0, false};
        flattenRecord(record, line);

        // string_view overload, never the variadic one — that instantiates fmt here and
        // breaks the build under /W4 /WX.
        spdlog::default_logger_raw()->log(toSpdlogLevel(record.level),
                                          spdlog::string_view_t{line.data, line.size});
    }

    static std::string_view baseName(std::string_view path) {
        const auto slash = path.find_last_of("/\\");
        return slash == std::string_view::npos ? path : path.substr(slash + 1);
    }

    // source_location::function_name() is the whole signature, not the name — MSVC gives
    // "void __cdecl TestFunction(void)". Keep the (possibly qualified) name and drop the
    // return type, calling convention and parameters; the sink renders the "()".
    static std::string_view shortFunctionName(std::string_view signature) {
        const auto paren = signature.find('(');
        if (paren == std::string_view::npos) {
            return signature;
        }

        const auto isNameChar = [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                   || c == '_' || c == ':' || c == '~' || c == '<' || c == '>';
        };

        std::size_t begin = paren;
        while (begin > 0 && isNameChar(signature[begin - 1])) {
            --begin;
        }

        // `operator ()` and other shapes where the char before '(' isn't part of a name: the
        // full signature beats an empty string.
        return begin == paren ? signature : signature.substr(begin, paren - begin);
    }

    void initLogging() {
        if (g_initialized.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        spdlog::set_pattern("[%H:%M:%S.%e][%^%l%$] %v");
        spdlog::set_level(spdlog::level::trace); // we filter; spdlog must not double-filter
    }

    void shutdownLogging() {
        if (!g_initialized.exchange(false, std::memory_order_acq_rel)) {
            return;
        }
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

    bool isEnabled(Level level) {
        return level >= g_minLevel.load(std::memory_order_relaxed) && level != Level::Off;
    }

    LogSinkFn setLogSink(LogSinkFn sink) {
        return g_sink.exchange(sink != nullptr ? sink : &spdlogSink, std::memory_order_acq_rel);
    }

    void setDiagnosticFrame(std::uint64_t frame) {
        g_frame.store(frame, std::memory_order_relaxed);
    }

    namespace detail {
        void logDispatch(Level level, const std::source_location& loc, std::string_view fmtStr,
                         std::format_args args) {
            // Backstop for the compile-time gate. A macro cannot be made private (its body
            // compiles with the caller's rights), so instead of trusting nobody calls
            // TE_LOG_PRIVATE_EMIT, the floor is re-checked here: bypassing the gate costs
            // performance, never correctness — no Trace line escapes into a Release build.
            if (static_cast<int>(level) < TE_LOG_ACTIVE_LEVEL) {
                return;
            }

            // TODO(S2-T3): per-channel level lookup. ADR-011 §2.
            if (!isEnabled(level)) {
                return;
            }

            std::array<char, MESSAGE_CAPACITY> storage{};
            LogFormatBuffer buffer{storage.data(), storage.size(), 0, false};

            try {
                std::vformat_to(LogFormatBufferIterator{buffer}, fmtStr, args);
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

            if (buffer.truncated) {
                const std::size_t start = buffer.size > TRUNCATION_MARKER.size()
                                              ? buffer.size - TRUNCATION_MARKER.size()
                                              : 0;
                buffer.size = start;
                for (const char c: TRUNCATION_MARKER) {
                    buffer.push(c);
                }
            }

            const LogRecord record{
                .frame = g_frame.load(std::memory_order_relaxed),
                .level = level,
                .channel = DEFAULT_CHANNEL,
                .message = std::string_view{buffer.data, buffer.size},
                .file = baseName(loc.file_name()),
                .function = shortFunctionName(loc.function_name()),
                .line = static_cast<std::uint32_t>(loc.line()), // line() is uint_least32_t
            };

            const LogSinkFn sink = g_sink.load(std::memory_order_acquire);
            if (sink != nullptr) {
                sink(record);
            }
        }
    }
}
