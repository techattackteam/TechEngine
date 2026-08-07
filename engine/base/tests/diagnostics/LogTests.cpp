#include <TechEngine/base/diagnostics/Log.hpp>

#include "diagnostics/FormatBuffer.hpp"
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

struct CapturedRecord {
    std::chrono::system_clock::time_point time;
    std::uint64_t frame;
    TechEngine::Level level;
    TechEngine::LogModule moduleTag;
    TechEngine::LogChannel channel;
    std::string message;
    std::string file;
    std::string function;
    std::uint32_t line;
};

static std::vector<CapturedRecord> g_captured;

static void captureSink(const TechEngine::LogRecord& record) {
    g_captured.push_back(CapturedRecord{record.time, record.frame, record.level, record.moduleTag, record.channel, std::string{record.message}, std::string{record.file}, std::string{record.function}, record.line});
}

static std::vector<std::string> g_secondary;

static void secondarySink(const TechEngine::LogRecord& record) {
    g_secondary.push_back(std::string{record.message});
}

// Diagnostics state is process-global and Catch2 shares one process — restore it or cases
// leak into each other.
class SinkGuard {
public:
    SinkGuard() : m_previousLevel{TechEngine::minLevel()} {
        g_captured.clear();
        TechEngine::addLogSink(&captureSink);
    }

    ~SinkGuard() {
        TechEngine::removeLogSink(&captureSink);
        TechEngine::setMinLevel(m_previousLevel);
        g_captured.clear();
    }

    SinkGuard(const SinkGuard&) = delete;

    SinkGuard& operator=(const SinkGuard&) = delete;

private:
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

// The gate is per-config — a binary can only assert the config it was built in.
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

TEST_CASE("two modules log on distinct channels", "[base][log][channel]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const TechEngine::LogModule client = TechEngine::registerLogModule("client");
    const TechEngine::LogModule net = TechEngine::registerLogModule("net");
    const TechEngine::LogChannel render = TechEngine::registerLogChannel("render", client);
    const TechEngine::LogChannel socket = TechEngine::registerLogChannel("socket", net);

    TE_LOGGER_INFO_CH(render, "frame {0}", 1);
    TE_LOGGER_INFO_CH(socket, "peer {0}", 2);

    REQUIRE_FALSE(render == socket);
    REQUIRE(g_captured.size() == 2);

    REQUIRE(g_captured[0].channel == render);
    REQUIRE(g_captured[0].moduleTag == client);
    REQUIRE(g_captured[1].channel == socket);
    REQUIRE(g_captured[1].moduleTag == net);

    REQUIRE(TechEngine::logChannelName(render) == "render");
    REQUIRE(TechEngine::logChannelName(socket) == "socket");
    REQUIRE(TechEngine::logModuleName(client) == "client");
    REQUIRE(TechEngine::logModuleName(net) == "net");
}

TEST_CASE("per-channel level filters independently", "[base][log][channel]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const TechEngine::LogModule audio = TechEngine::registerLogModule("audio");
    const TechEngine::LogChannel quiet = TechEngine::registerLogChannel("mixer", audio);
    const TechEngine::LogChannel loud = TechEngine::registerLogChannel("device", audio);
    TechEngine::setChannelLevel(quiet, TechEngine::Level::Warn);

    TE_LOGGER_INFO_CH(quiet, "dropped");
    TE_LOGGER_INFO_CH(loud, "kept");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].message == "kept");
    REQUIRE(g_captured[0].channel == loud);
    REQUIRE(TechEngine::channelLevel(quiet) == TechEngine::Level::Warn);
    REQUIRE(TechEngine::channelLevel(loud) == TechEngine::Level::Trace);
}

// Info rather than Trace on purpose: Trace is compiled out in Release, which would make the
// case pass for the wrong reason on that leg.
TEST_CASE("module level is a floor over its channels", "[base][log][channel]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const TechEngine::LogModule physics = TechEngine::registerLogModule("physics");
    const TechEngine::LogChannel solver = TechEngine::registerLogChannel("solver", physics, TechEngine::Level::Trace);
    TechEngine::setModuleLevel(physics, TechEngine::Level::Error);

    TE_LOGGER_INFO_CH(solver, "dropped by the module floor");
    TE_LOGGER_ERROR_CH(solver, "kept");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].level == TechEngine::Level::Error);
    REQUIRE(TechEngine::channelLevel(solver) == TechEngine::Level::Trace);
}

TEST_CASE("unregistered channel falls back to the default", "[base][log][channel]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const TechEngine::LogChannel bogus{9999};

    TE_LOGGER_INFO_CH(bogus, "still delivered");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].message == "still delivered");

    // The record keeps the handle it was given; only the lookup falls back, so an out-of-range
    // id can never index the table.
    REQUIRE(g_captured[0].channel == bogus);
    REQUIRE(g_captured[0].moduleTag == TechEngine::DEFAULT_MODULE);
    REQUIRE(TechEngine::logChannelName(bogus) == "default");
    REQUIRE(TechEngine::channelLevel(bogus) == TechEngine::channelLevel(TechEngine::DEFAULT_CHANNEL));
}

TEST_CASE("sinks are added, not swapped", "[base][log][sink]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);
    g_secondary.clear();

    REQUIRE(TechEngine::addLogSink(&secondarySink));

    TE_LOGGER_INFO("both");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_secondary.size() == 1);

    REQUIRE(TechEngine::removeLogSink(&secondarySink));

    TE_LOGGER_INFO("only the survivor");

    REQUIRE(g_captured.size() == 2);
    REQUIRE(g_secondary.size() == 1);

    g_secondary.clear();
}

// stderr is a process-wide descriptor: freopen can point it at a file, but nothing portable
// names the original to freopen back to, so the descriptor is saved and restored by hand.
template<typename Body>
static std::string captureStderr(Body&& body) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "te_log_stderr.txt";

    std::fflush(stderr);
#if defined(_WIN32)
    const int saved = _dup(_fileno(stderr));
#else
    const int saved = dup(fileno(stderr));
#endif
    REQUIRE(saved >= 0);
    // freopen_s on MSVC only because /WX turns std::freopen's C4996 into an error.
#if defined(_WIN32)
    FILE* redirected = nullptr;
    REQUIRE(freopen_s(&redirected, path.string().c_str(), "w", stderr) == 0);
#else
    REQUIRE(std::freopen(path.string().c_str(), "w", stderr) != nullptr);
#endif

    body();

    std::fflush(stderr);
#if defined(_WIN32)
    _dup2(saved, _fileno(stderr));
    _close(saved);
#else
    dup2(saved, fileno(stderr));
    close(saved);
#endif

    std::ifstream file{path};
    const std::string contents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

    std::error_code ec;
    std::filesystem::remove(path, ec);
    return contents;
}

// No SinkGuard — an empty sink set is the whole point. initLogging() is never called from the
// suite, so nothing else is installed either.
TEST_CASE("with no sink installed a record still reaches stderr", "[base][log][sink]") {
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const std::string captured = captureStderr([] { TE_LOGGER_ERROR("fallback {0}", 7); });

    TechEngine::setMinLevel(previous);

    REQUIRE(captured.find("fallback 7") != std::string::npos);
    REQUIRE(captured.find("LogTests.cpp") != std::string::npos);
}

TEST_CASE("a record carries a wall-clock stamp", "[base][log]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_INFO("first");
    TE_LOGGER_INFO("second");

    REQUIRE(g_captured.size() == 2);
    REQUIRE(g_captured[0].time.time_since_epoch().count() > 0);
    REQUIRE(g_captured[1].time >= g_captured[0].time);
}

static TechEngine::LogRecord sampleRecord(std::string_view message) {
    return TechEngine::LogRecord{
        .time = std::chrono::system_clock::now(),
        .frame = 1043,
        .level = TechEngine::Level::Info,
        .moduleTag = TechEngine::DEFAULT_MODULE,
        .channel = TechEngine::DEFAULT_CHANNEL,
        .message = message,
        .file = "renderer.cpp",
        .function = "renderScene",
        .line = 88,
    };
}

// [14:32:07.412][f 1043][module/channel][renderer.cpp:88:renderScene()][INFO] message
TEST_CASE("flatten renders the canonical line", "[base][log][format]") {
    std::array<char, 512> out{};
    const TechEngine::LogRecord record = sampleRecord("swapchain 1920x1080");

    const std::size_t size = TechEngine::detail::flattenRecord(record, out.data(), out.size());
    const std::string line{out.data(), size};

    REQUIRE(size > 0);
    REQUIRE(size <= out.size());

    // The clock is now(), so the stamp's shape is what can be asserted, not its value.
    REQUIRE(line.size() > 14);
    REQUIRE(line[0] == '[');
    REQUIRE(line[3] == ':');
    REQUIRE(line[6] == ':');
    REQUIRE(line[9] == '.');
    REQUIRE(line[13] == ']');

    const std::size_t frame = line.find("[f 1043]");
    const std::size_t channel = line.find("[default/default]");
    const std::size_t site = line.find("[renderer.cpp:88:renderScene()]");
    const std::size_t level = line.find("[INFO]");

    REQUIRE(frame != std::string::npos);
    REQUIRE(channel != std::string::npos);
    REQUIRE(site != std::string::npos);
    REQUIRE(level != std::string::npos);

    // Field order is the contract — a sink or a log grep reads positionally.
    REQUIRE(frame == 14);
    REQUIRE(frame < channel);
    REQUIRE(channel < site);
    REQUIRE(site < level);
    REQUIRE(line.ends_with("] swapchain 1920x1080"));
}

TEST_CASE("flatten marks its own truncation", "[base][log][format]") {
    std::array<char, 64> out{};
    const std::string huge(4096, 'x');
    const TechEngine::LogRecord record = sampleRecord(huge);

    const std::size_t size = TechEngine::detail::flattenRecord(record, out.data(), out.size());
    const std::string line{out.data(), size};

    REQUIRE(size <= out.size());
    REQUIRE(line.ends_with(TechEngine::detail::TRUNCATION_MARKER));
}

TEST_CASE("flatten never writes past its buffer", "[base][log][format]") {
    // One allocation, split by hand: the tail is the canary, so an overrun is observable
    // without relying on two arrays happening to be adjacent.
    std::array<char, 112> storage{};
    constexpr std::size_t capacity = 96;
    const std::string huge(4096, 'y');
    const TechEngine::LogRecord record = sampleRecord(huge);

    const std::size_t size = TechEngine::detail::flattenRecord(record, storage.data(), capacity);

    REQUIRE(size <= capacity);
    for (std::size_t i = capacity; i < storage.size(); i++) {
        REQUIRE(storage[i] == '\0');
    }
}

// Last on purpose: it exhausts the channel table, and registration is never unwound. Under
// ctest each case is its own process, so this only constrains a direct exe run.
TEST_CASE("registry overflow degrades to the default channel", "[base][log][channel]") {
    const SinkGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const TechEngine::LogModule owner = TechEngine::registerLogModule("overflow");
    const TechEngine::LogChannel first = TechEngine::registerLogChannel("first", owner);
    REQUIRE_FALSE(first == TechEngine::DEFAULT_CHANNEL);

    TechEngine::LogChannel last = first;
    for (int i = 0; i < 128; i++) {
        last = TechEngine::registerLogChannel("filler", owner);
    }

    REQUIRE(last == TechEngine::DEFAULT_CHANNEL);
    REQUIRE(TechEngine::logChannelName(first) == "first");

    // A full table degrades the channel, never the log line.
    TE_LOGGER_INFO_CH(last, "still delivered");

    REQUIRE(g_captured.size() == 1);
    REQUIRE(g_captured[0].message == "still delivered");
}

// The ring is always-on process-global state, not a pluggable sink — it has no guard of its
// own to add/remove, so each ring case resets it directly.
TEST_CASE("the ring captures records even with no sink installed", "[base][log][ring]") {
    TechEngine::ringClear();
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_WARN("no sink needed");

    TechEngine::setMinLevel(previous);

    std::array<TechEngine::LogRecord, 1> snapshot{};
    const std::size_t count = TechEngine::ringSnapshot(snapshot.data(), snapshot.size());

    REQUIRE(count == 1);
    REQUIRE(snapshot[0].message == "no sink needed");
    REQUIRE(snapshot[0].level == TechEngine::Level::Warn);
}

TEST_CASE("the ring keeps the last LOG_RING_CAPACITY records, oldest evicted first", "[base][log][ring]") {
    TechEngine::ringClear();
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const int total = static_cast<int>(TechEngine::LOG_RING_CAPACITY) + 5;
    for (int i = 0; i < total; i++) {
        TE_LOGGER_INFO("record {0}", i);
    }

    TechEngine::setMinLevel(previous);

    std::array<TechEngine::LogRecord, TechEngine::LOG_RING_CAPACITY> snapshot{};
    const std::size_t count = TechEngine::ringSnapshot(snapshot.data(), snapshot.size());

    REQUIRE(count == TechEngine::LOG_RING_CAPACITY);
    REQUIRE(snapshot[0].message == "record 5");
    REQUIRE(snapshot[count - 1].message == "record " + std::to_string(total - 1));
}

TEST_CASE("ringSnapshot never writes past the caller's capacity", "[base][log][ring]") {
    TechEngine::ringClear();
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_INFO("one");
    TE_LOGGER_INFO("two");
    TE_LOGGER_INFO("three");

    TechEngine::setMinLevel(previous);

    std::array<TechEngine::LogRecord, 2> snapshot{};
    const std::size_t count = TechEngine::ringSnapshot(snapshot.data(), snapshot.size());

    REQUIRE(count == 2);
    REQUIRE(snapshot[0].message == "two");
    REQUIRE(snapshot[1].message == "three");
}