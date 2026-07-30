#include <TechEngine/base/Assert.hpp>
#include <TechEngine/base/Log.hpp>

#include "FormatBuffer.hpp"
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct CapturedAssert {
    TechEngine::AssertKind kind;
    std::string condition;
    std::string message;
    std::string file;
    std::string function;
    std::uint32_t line;
};

static std::vector<CapturedAssert> g_fired;

// `message` points into assertDispatch's stack buffer — copying is the point of the test,
// not incidental. Returning {false, false} keeps a fatal TE_CHECK from killing the runner.
static TechEngine::AssertResponse captureHandler(const TechEngine::AssertContext& context) {
    g_fired.push_back(CapturedAssert{context.kind, std::string{context.condition}, std::string{context.message}, std::string{context.file}, std::string{context.function}, context.line});
    return TechEngine::AssertResponse{false, false};
}

// Diagnostics state is process-global (ADR-011 §8) and Catch2 shares one process — restore
// it or cases leak into each other.
class AssertHandlerGuard {
public:
    AssertHandlerGuard() : m_previous{TechEngine::setAssertHandler(&captureHandler)} {
        g_fired.clear();
    }

    ~AssertHandlerGuard() {
        TechEngine::setAssertHandler(m_previous);
        g_fired.clear();
    }

    AssertHandlerGuard(const AssertHandlerGuard&) = delete;

    AssertHandlerGuard& operator=(const AssertHandlerGuard&) = delete;

private:
    TechEngine::AssertHandlerFn m_previous;
};

struct CapturedLog {
    TechEngine::Level level;
    std::string message;
    std::string file;
    std::string function;
    std::uint32_t line;
};

static std::vector<CapturedLog> g_loggedRecords;

// Copies out immediately — record.message points into the dispatcher's stack buffer and does
// not outlive this call.
static void captureLogSink(const TechEngine::LogRecord& record) {
    g_loggedRecords.push_back(CapturedLog{record.level, std::string{record.message}, std::string{record.file}, std::string{record.function}, record.line});
}

class LogCaptureGuard {
public:
    LogCaptureGuard() {
        g_loggedRecords.clear();
        TechEngine::addLogSink(&captureLogSink);
    }

    ~LogCaptureGuard() {
        TechEngine::removeLogSink(&captureLogSink);
        g_loggedRecords.clear();
    }

    LogCaptureGuard(const LogCaptureGuard&) = delete;

    LogCaptureGuard& operator=(const LogCaptureGuard&) = delete;
};

TEST_CASE("setAssertHandler returns the previous handler", "[base][assert]") {
    const TechEngine::AssertHandlerFn original = TechEngine::setAssertHandler(&captureHandler);

    REQUIRE(TechEngine::assertHandler() == &captureHandler);
    REQUIRE(TechEngine::setAssertHandler(original) == &captureHandler);
    REQUIRE(TechEngine::assertHandler() == original);
}

TEST_CASE("a null handler restores the default rather than crashing", "[base][assert]") {
    const TechEngine::AssertHandlerFn original = TechEngine::setAssertHandler(nullptr);

    REQUIRE(TechEngine::assertHandler() == &TechEngine::defaultAssertHandler);

    TechEngine::setAssertHandler(original);
}

TEST_CASE("a passing condition never reaches the handler", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_ASSERT(true);
    TE_CHECK(true);

    REQUIRE(TE_VERIFY(true));
    REQUIRE(TE_ENSURE(true));
    REQUIRE(g_fired.empty());
}

TEST_CASE("each tier reports its own kind", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_CHECK(false);
    TE_VERIFY(false);
    TE_ENSURE(false);

    REQUIRE(g_fired.size() == 3);
    REQUIRE(g_fired[0].kind == TechEngine::AssertKind::Check);
    REQUIRE(g_fired[1].kind == TechEngine::AssertKind::Verify);
    REQUIRE(g_fired[2].kind == TechEngine::AssertKind::Ensure);
}

TEST_CASE("the reported condition is the source text", "[base][assert]") {
    const AssertHandlerGuard guard;

    const int x = 4;
    TE_CHECK(x == 11);

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().condition == "x == 11");
}

TEST_CASE("the failure carries its call site", "[base][assert]") {
    const AssertHandlerGuard guard;

    const std::uint32_t expected = static_cast<std::uint32_t>(__LINE__) + 1;
    TE_CHECK(false);

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().line == expected);
    REQUIRE(g_fired.front().file == "AssertTests.cpp");
}

TEST_CASE("the message is formatted with the logger's positional args", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_CHECK(false, "{1} then {0}", 7, "first");

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().message == "first then 7");
}

TEST_CASE("a message-less failure reports an empty message", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_CHECK(false);

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().message.empty());
}

TEST_CASE("an overlong message is truncated, not silently cut", "[base][assert]") {
    const AssertHandlerGuard guard;

    const std::string huge(4096, 'x');
    TE_CHECK(false, "{0}", huge);

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().message.ends_with(TechEngine::detail::TRUNCATION_MARKER));
}

TEST_CASE("a runtime format error is reported instead of escaping", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_CHECK(false, "{0:{1}}", 1, -1);

    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().message.starts_with("<format error: "));
}

TEST_CASE("TE_VERIFY evaluates its condition and yields its value", "[base][assert]") {
    const AssertHandlerGuard guard;

    int calls = 0;
    const auto sideEffect = [&calls](bool result) {
        ++calls;
        return result;
    };

    REQUIRE(TE_VERIFY(sideEffect(true)));
    REQUIRE(calls == 1);

    REQUIRE(!TE_VERIFY(sideEffect(false)));
    REQUIRE(calls == 2);
    REQUIRE(g_fired.size() == 1);
}

TEST_CASE("TE_ASSERT follows the per-config gate", "[base][assert]") {
    const AssertHandlerGuard guard;

    int calls = 0;

    [[maybe_unused]] const auto sideEffect = [&calls] {
        ++calls;
        return false;
    };

    TE_ASSERT(sideEffect());

#if TE_ASSERT_DEV
    REQUIRE(calls == 1);
    REQUIRE(g_fired.size() == 1);
    REQUIRE(g_fired.front().kind == TechEngine::AssertKind::Assert);
#else
    // Not just "does not report" — the condition must not run at all, which is the whole
    // reason TE_VERIFY exists as a separate tier.
    REQUIRE(calls == 0);
    REQUIRE(g_fired.empty());
#endif
}

TEST_CASE("TE_ENSURE yields its value and reports once per call site", "[base][assert]") {
    const AssertHandlerGuard guard;

    for (int i = 0; i < 5; ++i) {
        REQUIRE(!TE_ENSURE(false));
    }

    REQUIRE(g_fired.size() == 1);
}

// GOTCHA: the report-once static is per call site and lives for the process, so every
// TE_ENSURE below is a one-shot — a new case cannot reuse an existing call site and must
// write its own TE_ENSURE line.
TEST_CASE("two TE_ENSURE call sites report independently", "[base][assert]") {
    const AssertHandlerGuard guard;

    TE_ENSURE(false);
    TE_ENSURE(false);

    REQUIRE(g_fired.size() == 2);
}

TEST_CASE("kind fatality matches the config table", "[base][assert]") {
    REQUIRE(TechEngine::assertKindIsFatal(TechEngine::AssertKind::Check));
    REQUIRE(!TechEngine::assertKindIsFatal(TechEngine::AssertKind::Ensure));

#if TE_ASSERT_DEV
    REQUIRE(TechEngine::assertKindIsFatal(TechEngine::AssertKind::Assert));
    REQUIRE(TechEngine::assertKindIsFatal(TechEngine::AssertKind::Verify));
#else
    REQUIRE(!TechEngine::assertKindIsFatal(TechEngine::AssertKind::Assert));
    REQUIRE(!TechEngine::assertKindIsFatal(TechEngine::AssertKind::Verify));
#endif
}

TEST_CASE("the default handler asks to abort exactly for fatal kinds", "[base][assert]") {
    const TechEngine::AssertContext check{TechEngine::AssertKind::Check, "cond", "", "AssertTests.cpp", "test", 1};
    const TechEngine::AssertContext ensure{TechEngine::AssertKind::Ensure, "cond", "", "AssertTests.cpp", "test", 1};

    REQUIRE(TechEngine::defaultAssertHandler(check).abortProcess);
    REQUIRE(!TechEngine::defaultAssertHandler(ensure).abortProcess);
    REQUIRE(!TechEngine::defaultAssertHandler(check).breakAtCallSite);
}

static int g_handlerEntries = 0;

// Both raises are TE_ENSURE on purpose. The guarded path still reports the nested failure
// as fatal for a fatal tier (ADR-011 §7), so a nested TE_CHECK aborts the runner rather
// than failing the case — Ensure is the only tier that is never fatal.
static TechEngine::AssertResponse recursingHandler(const TechEngine::AssertContext&) {
    ++g_handlerEntries;
    if (g_handlerEntries < 5) {
        TE_ENSURE(false, "raised while reporting");
    }
    return TechEngine::AssertResponse{false, false};
}

TEST_CASE("a failure raised while reporting does not re-enter the handler", "[base][assert]") {
    g_handlerEntries = 0;
    const TechEngine::AssertHandlerFn original = TechEngine::setAssertHandler(&recursingHandler);

    TE_ENSURE(false, "outer");

    TechEngine::setAssertHandler(original);

    REQUIRE(g_handlerEntries == 1);
}

struct ThrownFromHandler {};

static TechEngine::AssertResponse throwingHandler(const TechEngine::AssertContext&) {
    throw ThrownFromHandler{};
}

TEST_CASE("a throwing handler does not latch the in-flight guard", "[base][assert]") {
    const TechEngine::AssertHandlerFn original = TechEngine::setAssertHandler(&throwingHandler);

    REQUIRE_THROWS_AS(TE_ENSURE(false, "thrown"), ThrownFromHandler);

    TechEngine::setAssertHandler(original);

    const AssertHandlerGuard guard;
    TE_ENSURE(false, "delivered after the throw");

    REQUIRE(g_fired.size() == 1);
}

// ADR-011 §6/§7: a failure logs Critical through the Logger, flushes, then the response's
// abortProcess tells the macro whether to abort — this covers the log half; "the default
// handler asks to abort exactly for fatal kinds" already covers abortProcess itself.
TEST_CASE("the default handler routes the failure through the Logger at Critical", "[base][assert][log]") {
    const LogCaptureGuard logGuard;

    const TechEngine::AssertContext context{TechEngine::AssertKind::Ensure, "budget <= max", "over by 3", "AssertTests.cpp", "test", 42};
    const TechEngine::AssertResponse response = TechEngine::defaultAssertHandler(context);

    REQUIRE_FALSE(response.abortProcess);
    REQUIRE(g_loggedRecords.size() == 1);
    REQUIRE(g_loggedRecords[0].level == TechEngine::Level::Critical);
    REQUIRE(g_loggedRecords[0].file == "AssertTests.cpp");
    REQUIRE(g_loggedRecords[0].line == 42);
    REQUIRE(g_loggedRecords[0].message.find("ENSURE") != std::string::npos);
    REQUIRE(g_loggedRecords[0].message.find("budget <= max") != std::string::npos);
    REQUIRE(g_loggedRecords[0].message.find("over by 3") != std::string::npos);
}

TEST_CASE("a message-less failure still composes a readable Critical line", "[base][assert][log]") {
    const LogCaptureGuard logGuard;

    const TechEngine::AssertContext context{TechEngine::AssertKind::Check, "device != nullptr", "", "AssertTests.cpp", "test", 7};
    TechEngine::defaultAssertHandler(context);

    REQUIRE(g_loggedRecords.size() == 1);
    REQUIRE(g_loggedRecords[0].message == "[CHECK] (device != nullptr)");
}

// The ring is always-on (ADR-011 §3/§8) — not something the handler opts into — so a failure
// lands there even with no pluggable sink installed.
TEST_CASE("a failure's Critical log lands in the always-on ring", "[base][assert][log]") {
    TechEngine::ringClear();

    const TechEngine::AssertContext context{TechEngine::AssertKind::Check, "device != nullptr", "", "AssertTests.cpp", "test", 7};
    TechEngine::defaultAssertHandler(context);

    std::array<TechEngine::LogRecord, TechEngine::LOG_RING_CAPACITY> snapshot{};
    const std::size_t count = TechEngine::ringSnapshot(snapshot.data(), snapshot.size());

    REQUIRE(count >= 1);
    REQUIRE(snapshot[count - 1].level == TechEngine::Level::Critical);
    REQUIRE(snapshot[count - 1].line == 7);
}
