#pragma once

#include <TechEngine/base/diagnostics/FormatString.hpp>

#include <atomic>
#include <cstdint>
#include <format>
#include <source_location>
#include <string_view>

#if !defined(TE_ASSERT_DEV)
#define TE_ASSERT_DEV 1
#endif

namespace TechEngine {
    enum class AssertKind : std::uint8_t {
        Assert,
        Verify,
        Check,
        Ensure,
    };

    struct AssertContext {
        AssertKind kind = AssertKind::Assert;
        std::string_view condition;
        std::string_view message;
        std::string_view file;
        std::string_view function;
        std::uint32_t line = 0;
    };

    struct AssertResponse {
        bool breakAtCallSite = false;
        bool abortProcess = false;
    };

    using AssertHandlerFn = AssertResponse (*)(const AssertContext&);

    bool assertKindIsFatal(AssertKind kind);

    std::string_view assertKindName(AssertKind kind);

    AssertResponse defaultAssertHandler(const AssertContext& context);

    AssertHandlerFn setAssertHandler(AssertHandlerFn handler);

    AssertHandlerFn assertHandler();

    namespace detail {
        AssertResponse assertDispatch(AssertKind kind, std::string_view condition, const std::source_location& loc, std::string_view fmtStr, std::format_args args);

        AssertResponse assertReport(AssertKind kind, std::string_view condition, const std::source_location& loc);

        template<typename... Args>
        AssertResponse assertReport(AssertKind kind, std::string_view condition, const std::source_location& loc, PositionalFormat<Args...> fmtStr, Args&&... args) {
            return assertDispatch(kind, condition, loc, fmtStr.get(), std::make_format_args(args...));
        }

        [[noreturn]] void assertAbort();
    }
}

#if defined(_MSC_VER)
#define TE_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
#define TE_DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__)
#define TE_DEBUG_BREAK() __builtin_trap()
#else
#define TE_DEBUG_BREAK() ((void)0)
#endif

// TE_ASSERT_PRIVATE_* are plumbing for the four tiers below — they bypass the per-config
// gate, so a direct call ships a dev-only check into Release. Same enforcement as the
// Logger's: the name plus ci.yml → private-symbols.
//
// GOTCHA: the optional message rides __VA_OPT__, which needs a conforming preprocessor —
// MSVC gets it from /Zc:preprocessor in cmake/warnings.cmake. Drop that flag and the
// traditional preprocessor quietly swallows the empty __VA_ARGS__'s trailing comma, so
// Windows stays green while the Clang leg fails on every message-less assert.
// A condition the compiler can fold — TE_ASSERT(sizeof(T) == 4), or a const local in a
// test — is a legitimate assert, not the mistake C4127 is hunting for. Suppressed here so
// call sites never have to.
#if defined(_MSC_VER)
#define TE_ASSERT_PRIVATE_FOLDABLE_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4127))
#define TE_ASSERT_PRIVATE_FOLDABLE_END __pragma(warning(pop))
#else
#define TE_ASSERT_PRIVATE_FOLDABLE_BEGIN
#define TE_ASSERT_PRIVATE_FOLDABLE_END
#endif

#define TE_ASSERT_PRIVATE_RESPOND(response)                           \
    do {                                                              \
        const ::TechEngine::AssertResponse te_response_ = (response); \
        if (te_response_.breakAtCallSite) {                           \
            TE_DEBUG_BREAK();                                         \
        }                                                             \
        if (te_response_.abortProcess) {                              \
            ::TechEngine::detail::assertAbort();                      \
        }                                                             \
    } while (0)

#define TE_ASSERT_PRIVATE_STMT(kind, cond, ...)                                                                                                         \
    do {                                                                                                                                                \
        TE_ASSERT_PRIVATE_FOLDABLE_BEGIN                                                                                                                \
        if (!(cond)) [[unlikely]] {                                                                                                                     \
            TE_ASSERT_PRIVATE_RESPOND(::TechEngine::detail::assertReport((kind), #cond, ::std::source_location::current() __VA_OPT__(, ) __VA_ARGS__)); \
        }                                                                                                                                               \
        TE_ASSERT_PRIVATE_FOLDABLE_END                                                                                                                  \
    } while (0)

#define TE_ASSERT_PRIVATE_DISCARD(...) \
    do {                               \
    } while (0)

// Expression form: `cond` is evaluated exactly once and the result is the macro's value.
// source_location is taken in the argument list, not the body — inside the lambda it would
// name the lambda, not the failing line.
#define TE_ASSERT_PRIVATE_EXPR(kind, cond, ...)                                                                           \
    ([&](const ::std::source_location& te_loc_) -> bool {                                                                 \
        TE_ASSERT_PRIVATE_FOLDABLE_BEGIN                                                                                  \
        if ((cond)) [[likely]] {                                                                                          \
            return true;                                                                                                  \
        }                                                                                                                 \
        TE_ASSERT_PRIVATE_FOLDABLE_END                                                                                    \
        TE_ASSERT_PRIVATE_RESPOND(::TechEngine::detail::assertReport((kind), #cond, te_loc_ __VA_OPT__(, ) __VA_ARGS__)); \
        return false;                                                                                                     \
    }(::std::source_location::current()))

// The report-once static is inside the lambda body, so each expansion gets its own —
// that is what makes it per-call-site with no shared table.
#define TE_ASSERT_PRIVATE_ONCE(cond, ...)                                                                                                               \
    ([&](const ::std::source_location& te_loc_) -> bool {                                                                                               \
        TE_ASSERT_PRIVATE_FOLDABLE_BEGIN                                                                                                                \
        if ((cond)) [[likely]] {                                                                                                                        \
            return true;                                                                                                                                \
        }                                                                                                                                               \
        TE_ASSERT_PRIVATE_FOLDABLE_END                                                                                                                  \
        static ::std::atomic<bool> te_reported_ = false;                                                                                                \
        if (!te_reported_.exchange(true, ::std::memory_order_relaxed)) {                                                                                \
            TE_ASSERT_PRIVATE_RESPOND(::TechEngine::detail::assertReport(::TechEngine::AssertKind::Ensure, #cond, te_loc_ __VA_OPT__(, ) __VA_ARGS__)); \
        }                                                                                                                                               \
        return false;                                                                                                                                   \
    }(::std::source_location::current()))

// never put a side effect in TE_ASSERT — it vanishes in Release. That is what
// TE_VERIFY is for.
#if TE_ASSERT_DEV
#define TE_ASSERT(cond, ...) TE_ASSERT_PRIVATE_STMT(::TechEngine::AssertKind::Assert, cond, __VA_ARGS__)
#else
#define TE_ASSERT(cond, ...) TE_ASSERT_PRIVATE_DISCARD(cond)
#endif

#define TE_VERIFY(cond, ...) TE_ASSERT_PRIVATE_EXPR(::TechEngine::AssertKind::Verify, cond, __VA_ARGS__)

#define TE_CHECK(cond, ...) TE_ASSERT_PRIVATE_STMT(::TechEngine::AssertKind::Check, cond, __VA_ARGS__)

#define TE_ENSURE(cond, ...) TE_ASSERT_PRIVATE_ONCE(cond, __VA_ARGS__)
