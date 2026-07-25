#include <TechEngine/base/Log.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

struct CapturedRecord {
    std::uint64_t frame;
    TechEngine::Level level;
    std::string message;
    std::string file;
    std::string function;
    std::uint32_t line;
};

static std::vector<CapturedRecord> g_captured;

static void captureSink(const TechEngine::LogRecord& record) {
    g_captured.push_back(CapturedRecord{record.frame, record.level, std::string{record.message},
                                        std::string{record.file}, std::string{record.function},
                                        record.line});
}

// Diagnostics state is process-global (ADR-011 §8) and Catch2 shares one process — restore
// it or cases leak into each other.
class SinkGuard {
public:
    SinkGuard()
        : m_previousSink{TechEngine::setLogSink(&captureSink)},
          m_previousLevel{TechEngine::minLevel()} {
        g_captured.clear();
    }

    ~SinkGuard() {
        TechEngine::setLogSink(m_previousSink);
        TechEngine::setMinLevel(m_previousLevel);
        g_captured.clear();
    }

    SinkGuard(const SinkGuard&) = delete;

    SinkGuard& operator=(const SinkGuard&) = delete;

private:
    TechEngine::LogSinkFn m_previousSink;
    TechEngine::Level m_previousLevel;
};

TEST_CASE("positional args reorder and reuse", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_INFO("swapchain {1}x{0} (reuse {1})", 1080, 1920);

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].message == "swapchain 1920x1080 (reuse 1920)");
}

TEST_CASE("runtime min level suppresses below-threshold records", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Warn);

    TE_LOGGER_INFO("dropped {0}", 1);
    TE_LOGGER_WARN("kept {0}", 2);
    TE_LOGGER_ERROR("kept {0}", 3);

    REQUIRE(g_captured.size() == 2);
    REQUIRE(g_captured[0].level == TechEngine::Level::Warn);
    REQUIRE(g_captured[1].level == TechEngine::Level::Error);
}

TEST_CASE("Level::Off suppresses everything", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Off);

    TE_LOGGER_CRITICAL("not delivered");

    REQUIRE(g_captured.empty());
}

TEST_CASE("source_location reaches the record", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const std::uint32_t expectedLine = __LINE__ + 1;
    TE_LOGGER_INFO("located");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].line == expectedLine);
    REQUIRE(g_captured[0].file == "LogTests.cpp");
}

// MSVC hands source_location the whole signature ("void __cdecl f(void)"); the record must
// carry the bare name so a line reads main.cpp:7:TestFunction().
TEST_CASE("function name is trimmed to the identifier", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_INFO("named");

    REQUIRE(g_captured.size() == 1);
    const std::string& fn = g_captured[0].function;
    REQUIRE(fn.find(' ') == std::string::npos);
    REQUIRE(fn.find('(') == std::string::npos);
    REQUIRE(fn.find("__cdecl") == std::string::npos);
    REQUIRE_FALSE(fn.empty());
}

TEST_CASE("frame stamp is the value app pushed", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TechEngine::setDiagnosticFrame(1043);
    TE_LOGGER_INFO("stamped");
    TechEngine::setDiagnosticFrame(0);

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].frame == 1043);
}

TEST_CASE("macros survive a dangling else", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const bool condition = false;
    if (condition) {
        TE_LOGGER_INFO("then branch");
    } else {
        TE_LOGGER_WARN("else branch");
    }

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].level == TechEngine::Level::Warn);
}

TEST_CASE("over-long messages truncate rather than overflow", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const std::string huge(4096, 'x');
    TE_LOGGER_INFO("{0}", huge);

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].message.size() < huge.size());
    REQUIRE(g_captured[0].message.ends_with("[truncated]"));
}

// Per-config gate (ADR-011 §4) — a binary can only assert the config it was built in.
TEST_CASE("compile-time level gate matches the build config", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_TRACE("trace {0}", 1);

#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_TRACE
    REQUIRE(g_captured.size() == 1);
#else
    REQUIRE(g_captured.empty());
#endif
}