#include <TechEngine/base/Assert.hpp>

#include "FormatBuffer.hpp"
#include "SourceName.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>

namespace TechEngine {
    static constexpr std::size_t ASSERT_MESSAGE_CAPACITY = 1024;

    static std::atomic<AssertHandlerFn> g_assertHandler{&defaultAssertHandler};

    // ADR-011 §7 — a failure raised while already reporting skips the handler entirely and
    // goes straight to stderr, so a broken handler cannot recurse forever.
    static thread_local bool t_reporting = false;

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

        std::fprintf(stderr, "[%.*s][%.*s:%u:%.*s()] (%.*s)", static_cast<int>(kind.size()), kind.data(), static_cast<int>(context.file.size()), context.file.data(), context.line, static_cast<int>(context.function.size()), context.function.data(), static_cast<int>(context.condition.size()), context.condition.data());

        if (!context.message.empty()) {
            std::fprintf(stderr, " %.*s", static_cast<int>(context.message.size()), context.message.data());
        }

        std::fputc('\n', stderr);
        std::fflush(stderr);
    }

    AssertResponse defaultAssertHandler(const AssertContext& context) {
        // TODO(S2-T5): route through the Logger at Critical + flush all sinks before the
        // abort (ADR-011 §6). stderr-only until that lands, so `base` still reports.
        writeToStderr(context);
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
                kind, condition, std::string_view{storage.data(), buffer.size}, baseName(loc.file_name()), shortFunctionName(loc.function_name()), static_cast<std::uint32_t>(loc.line()),
            };

            if (t_reporting) {
                writeToStderr(context);
                return AssertResponse{false, assertKindIsFatal(kind)};
            }

            t_reporting = true;
            const AssertResponse response = assertHandler()(context);
            t_reporting = false;

            return response;
        }

        AssertResponse assertReport(AssertKind kind, std::string_view condition, const std::source_location& loc) {
            // GOTCHA: not std::format_args{} — the standard mandates that default constructor
            // but MSVC 14.44's STL does not provide it. An empty store converts on every
            // toolchain, and assertDispatch ignores args when fmtStr is empty anyway.
            return assertDispatch(kind, condition, loc, std::string_view{}, std::make_format_args());
        }

        void assertAbort() {
            std::abort();
        }
    }
}
