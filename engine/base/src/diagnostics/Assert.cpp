#include <TechEngine/base/diagnostics/Assert.hpp>

#include "FormatBuffer.hpp"
#include "LogInternal.hpp"
#include "SourceName.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>

namespace TechEngine {
    static constexpr std::size_t ASSERT_MESSAGE_CAPACITY = 1024;

    static std::atomic<AssertHandlerFn> g_assertHandler{&defaultAssertHandler};

    static thread_local bool t_reporting = false;

    struct ReportingScope {
        ReportingScope() {
            t_reporting = true;
        }

        ~ReportingScope() {
            t_reporting = false;
        }

        ReportingScope(const ReportingScope&) = delete;

        ReportingScope& operator=(const ReportingScope&) = delete;
    };

    bool assertKindIsFatal(AssertKind kind) {
        switch (kind) {
            case AssertKind::Assert:
            case AssertKind::Verify:
                return TE_ASSERT_DEV != 0;
            case AssertKind::Check:
                return true;
            case AssertKind::Ensure:
                return false;
        }
        return false;
    }

    std::string_view assertKindName(AssertKind kind) {
        switch (kind) {
            case AssertKind::Assert:
                return "ASSERT";
            case AssertKind::Verify:
                return "VERIFY";
            case AssertKind::Check:
                return "CHECK";
            case AssertKind::Ensure:
                return "ENSURE";
        }
        return "ASSERT";
    }

    static void writeToStderr(const AssertContext& context) {
        const std::string_view kind = assertKindName(context.kind);

        std::fprintf(
            stderr,
            "[%.*s][%.*s:%u:%.*s()] (%.*s)",
            static_cast<int>(kind.size()),
            kind.data(),
            static_cast<int>(context.file.size()),
            context.file.data(),
            context.line,
            static_cast<int>(context.function.size()),
            context.function.data(),
            static_cast<int>(context.condition.size()),
            context.condition.data());

        if (!context.message.empty()) {
            std::fprintf(stderr, " %.*s", static_cast<int>(context.message.size()), context.message.data());
        }

        std::fputc('\n', stderr);
        std::fflush(stderr);
    }

    static constexpr std::size_t CRITICAL_MESSAGE_CAPACITY = ASSERT_MESSAGE_CAPACITY + 128;

    // "[KIND] (cond) message" in one line — the Logger's LogRecord has no condition/kind
    // fields of its own, so the assert context is flattened into the record's message rather
    // than growing LogRecord for one caller. snprintf, not std::format: context.message is
    // already-formatted, arbitrary text and may itself contain '{'/'}'.
    static std::size_t composeCriticalMessage(const AssertContext& context, char* out, std::size_t capacity) {
        if (capacity == 0) {
            return 0;
        }

        const std::string_view kind = assertKindName(context.kind);
        const int written = context.message.empty()
                                ? std::snprintf(out, capacity, "[%.*s] (%.*s)", static_cast<int>(kind.size()), kind.data(), static_cast<int>(context.condition.size()), context.condition.data())
                                : std::snprintf(out, capacity, "[%.*s] (%.*s) %.*s", static_cast<int>(kind.size()), kind.data(), static_cast<int>(context.condition.size()), context.condition.data(), static_cast<int>(context.message.size()), context.message.data());

        if (written <= 0) {
            return 0;
        }
        return std::min(static_cast<std::size_t>(written), capacity - 1);
    }

    AssertResponse defaultAssertHandler(const AssertContext& context) {
        std::array<char, CRITICAL_MESSAGE_CAPACITY> messageStorage;
        const std::size_t messageSize = composeCriticalMessage(context, messageStorage.data(), messageStorage.size());

        detail::logRaw(Level::Critical, DEFAULT_CHANNEL, context.file, context.function, context.line, std::string_view{messageStorage.data(), messageSize});
        if (assertKindIsFatal(context.kind)) {
            flushLogs();
        }

        return AssertResponse{false, assertKindIsFatal(context.kind)};
    }

    AssertHandlerFn setAssertHandler(AssertHandlerFn handler) {
        return g_assertHandler.exchange(handler != nullptr ? handler : &defaultAssertHandler, std::memory_order_acq_rel);
    }

    AssertHandlerFn assertHandler() {
        return g_assertHandler.load(std::memory_order_acquire);
    }

    namespace detail {
        AssertResponse assertDispatch(AssertKind kind, std::string_view condition, const std::source_location& loc, std::string_view fmtStr, std::format_args args) {
            std::array<char, ASSERT_MESSAGE_CAPACITY> storage;
            FormatBuffer buffer{storage.data(), storage.size(), 0, false};

            if (!fmtStr.empty()) {
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
            }

            buffer.markTruncated();

            const AssertContext context{
                kind,
                condition,
                std::string_view{storage.data(), buffer.size},
                baseName(loc.file_name()),
                shortFunctionName(loc.function_name()),
                static_cast<std::uint32_t>(loc.line()),
            };

            if (t_reporting) {
                writeToStderr(context);
                return AssertResponse{false, assertKindIsFatal(kind)};
            }

            const ReportingScope scope;
            return assertHandler()(context);
        }

        AssertResponse assertReport(AssertKind kind, std::string_view condition, const std::source_location& loc) {
            return assertDispatch(kind, condition, loc, std::string_view{}, std::make_format_args());
        }

        void assertAbort() {
            std::abort();
        }
    }
}
