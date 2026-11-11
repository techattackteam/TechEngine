#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/LogError.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>
#include <TechEngine/testing/LogCapture.hpp>

#include <diagnostics/FormatBuffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <source_location>
#include <stop_token>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

static constexpr int LOGGING_THREADS = 4;
static constexpr int RECORDS_PER_THREAD = 500;
static constexpr std::size_t RECORDS_AFTER_REMOVAL = 1000;
static constexpr std::chrono::seconds LOGGER_TIMEOUT{5};

class CountingSink final : public TechEngine::LogSink {
public:
    void write(const TechEngine::LogRecord&) noexcept override {
        m_count.fetch_add(1, std::memory_order_relaxed);
    }

    std::size_t getCount() const {
        return m_count.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::size_t> m_count = 0;
};

// The sink table is process-global, so a sink must leave it even when a REQUIRE ends the case
// early; one left behind dangles for every later case in the same run.
class SinkRegistration {
public:
    explicit SinkRegistration(TechEngine::LogSink& sink) : m_sink(sink) {
        (void)TechEngine::addLogSink(m_sink);
    }

    ~SinkRegistration() {
        if (!m_removed) {
            (void)TechEngine::removeLogSink(m_sink);
        }
    }

    SinkRegistration(const SinkRegistration&) = delete;

    SinkRegistration& operator=(const SinkRegistration&) = delete;

    std::error_code remove() {
        m_removed = true;
        return TechEngine::removeLogSink(m_sink);
    }

private:
    TechEngine::LogSink& m_sink;
    bool m_removed = false;
};

template<typename Predicate>
static bool waitUntil(Predicate&& predicate) {
    const auto deadline = std::chrono::steady_clock::now() + LOGGER_TIMEOUT;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}

TEST_CASE("positional args reorder and reuse", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    TE_LOGGER_WARN("swapchain {1}x{0} (reuse {1})", 1080, 1920);

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].message == "swapchain 1920x1080 (reuse 1920)");
}

TEST_CASE("runtime min level suppresses below-threshold records", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Warn);

    TE_LOGGER_INFO("dropped {0}", 1);
    TE_LOGGER_WARN("kept {0}", 2);
    TE_LOGGER_ERROR("kept {0}", 3);

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 2);
    REQUIRE(captured[0].level == TechEngine::Level::Warn);
    REQUIRE(captured[1].level == TechEngine::Level::Error);
}

TEST_CASE("Level::Off suppresses everything", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;
    TechEngine::setMinLevel(TechEngine::Level::Off);

    TE_LOGGER_CRITICAL("not delivered");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.empty());
}

TEST_CASE("source_location reaches the record", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    const std::uint32_t expectedLine = __LINE__ + 1;
    TE_LOGGER_WARN("located");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].line == expectedLine);
    REQUIRE(captured[0].file == "LogTests.cpp");
}

// MSVC hands source_location the whole signature ("void __cdecl f(void)"); the record must
// carry the bare name so a line reads main.cpp:7:TestFunction().
TEST_CASE("function name is trimmed to the identifier", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    TE_LOGGER_WARN("named");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    const std::string_view fn = captured[0].function;
    REQUIRE(fn.find(' ') == std::string_view::npos);
    REQUIRE(fn.find('(') == std::string_view::npos);
    REQUIRE(fn.find("__cdecl") == std::string_view::npos);
    REQUIRE_FALSE(fn.empty());
}

TEST_CASE("tick stamp is the value app pushed", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    TechEngine::setDiagnosticTick(1043);
    TE_LOGGER_WARN("stamped");
    TechEngine::setDiagnosticTick(0);

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].tick == 1043);
}

TEST_CASE("macros survive a dangling else", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    const bool condition = false;
    if (condition) {
        TE_LOGGER_INFO("then branch");
    } else {
        TE_LOGGER_WARN("else branch");
    }

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].level == TechEngine::Level::Warn);
}

TEST_CASE("over-long messages truncate rather than overflow", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    const std::string huge(4096, 'x');
    TE_LOGGER_WARN("{0}", huge);

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].message.size() < huge.size());
    REQUIRE(captured[0].message.ends_with("[truncated]"));
}

// The gate is per-config: a binary can only assert the config it was built in.
TEST_CASE("compile-time level gate matches the build config", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    TE_LOGGER_TRACE("trace {0}", 1);

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
#if TE_LOG_ACTIVE_LEVEL <= TE_LOG_LEVEL_TRACE
    REQUIRE(captured.size() == 1);
#else
    REQUIRE(captured.empty());
#endif
}

// The macros gate on this TU's TE_LOG_ACTIVE_LEVEL and Log.cpp gates on the one the library
// was compiled with. Both come from the same PUBLIC define, so they agree only while it
// reaches both, and a TU that fell back to the header's default instead fails silently.
// Dispatching past the macros is the only way to see the library's half of the gate.
TEST_CASE("the library gates at the level this TU compiled", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    for (int level = TE_LOG_LEVEL_TRACE; level <= TE_LOG_LEVEL_CRITICAL; level++) {
        TechEngine::internal::logImpl(static_cast<TechEngine::Level>(level), TechEngine::DEFAULT_CHANNEL, std::source_location::current(), "{0}", level);
    }

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == static_cast<std::size_t>(TE_LOG_LEVEL_CRITICAL - TE_LOG_ACTIVE_LEVEL + 1));
}

TEST_CASE("two modules log on distinct channels", "[base][log][channel]") {
    const TechEngineTests::LogCaptureGuard guard;

    const TechEngine::LogModule client = TechEngine::registerLogModule("client");
    const TechEngine::LogModule net = TechEngine::registerLogModule("net");
    const TechEngine::LogChannel render = TechEngine::registerLogChannel("render", client);
    const TechEngine::LogChannel socket = TechEngine::registerLogChannel("socket", net);

    TE_LOGGER_WARN_CH(render, "frame {0}", 1);
    TE_LOGGER_WARN_CH(socket, "peer {0}", 2);

    REQUIRE_FALSE(render == socket);
    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 2);

    REQUIRE(captured[0].channel == render);
    REQUIRE(captured[0].moduleTag == client);
    REQUIRE(captured[1].channel == socket);
    REQUIRE(captured[1].moduleTag == net);

    REQUIRE(TechEngine::logChannelName(render) == "render");
    REQUIRE(TechEngine::logChannelName(socket) == "socket");
    REQUIRE(TechEngine::logModuleName(client) == "client");
    REQUIRE(TechEngine::logModuleName(net) == "net");
}

TEST_CASE("per-channel level filters independently", "[base][log][channel]") {
    const TechEngineTests::LogCaptureGuard guard;

    const TechEngine::LogModule audio = TechEngine::registerLogModule("audio");
    const TechEngine::LogChannel quiet = TechEngine::registerLogChannel("mixer", audio);
    const TechEngine::LogChannel loud = TechEngine::registerLogChannel("device", audio);
    TechEngine::setChannelLevel(quiet, TechEngine::Level::Error);

    TE_LOGGER_WARN_CH(quiet, "dropped");
    TE_LOGGER_WARN_CH(loud, "kept");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].message == "kept");
    REQUIRE(captured[0].channel == loud);
    REQUIRE(TechEngine::channelLevel(quiet) == TechEngine::Level::Error);
    REQUIRE(TechEngine::channelLevel(loud) == TechEngine::Level::Trace);
}

// Info rather than Trace on purpose: Trace is compiled out in Release, which would make the
// case pass for the wrong reason on that leg.
TEST_CASE("module level is a floor over its channels", "[base][log][channel]") {
    const TechEngineTests::LogCaptureGuard guard;

    const TechEngine::LogModule physics = TechEngine::registerLogModule("physics");
    const TechEngine::LogChannel solver = TechEngine::registerLogChannel("solver", physics, TechEngine::Level::Trace);
    TechEngine::setModuleLevel(physics, TechEngine::Level::Error);

    TE_LOGGER_INFO_CH(solver, "dropped by the module floor");
    TE_LOGGER_ERROR_CH(solver, "kept");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].level == TechEngine::Level::Error);
    REQUIRE(TechEngine::channelLevel(solver) == TechEngine::Level::Trace);
}

TEST_CASE("unregistered channel falls back to the default", "[base][log][channel]") {
    const TechEngineTests::LogCaptureGuard guard;

    const TechEngine::LogChannel bogus{9999};

    TE_LOGGER_WARN_CH(bogus, "still delivered");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].message == "still delivered");

    // The record keeps the handle it was given; only the lookup falls back, so an out-of-range
    // id can never index the table.
    REQUIRE(captured[0].channel == bogus);
    REQUIRE(captured[0].moduleTag == TechEngine::DEFAULT_MODULE);
    REQUIRE(TechEngine::logChannelName(bogus) == "default");
    REQUIRE(TechEngine::channelLevel(bogus) == TechEngine::channelLevel(TechEngine::DEFAULT_CHANNEL));
}

TEST_CASE("sinks are added, not swapped", "[base][log][sink]") {
    const TechEngineTests::LogCaptureGuard survivor;
    TechEngineTests::LogCaptureGuard removed;

    TE_LOGGER_WARN("both");

    REQUIRE(survivor.getRecords().size() == 1);
    REQUIRE(removed.getRecords().size() == 1);

    REQUIRE(TechEngine::removeLogSink(removed) == std::error_code{});

    TE_LOGGER_WARN("only the survivor");

    REQUIRE(survivor.getRecords().size() == 2);
    REQUIRE(removed.getRecords().size() == 1);
}

TEST_CASE("two sinks see the same records in the same order", "[base][log][sink][threads]") {
    const TechEngineTests::LogCaptureGuard first;
    const TechEngineTests::LogCaptureGuard second;

    std::vector<std::jthread> loggers;
    for (int thread = 0; thread < LOGGING_THREADS; thread++) {
        loggers.emplace_back([thread] {
            for (int record = 0; record < RECORDS_PER_THREAD; record++) {
                TE_LOGGER_WARN("thread {0} record {1}", thread, record);
            }
        });
    }
    for (std::jthread& logger: loggers) {
        logger.join();
    }

    const std::vector<std::string> firstMessages = first.getMessages();
    REQUIRE(firstMessages.size() == static_cast<std::size_t>(LOGGING_THREADS * RECORDS_PER_THREAD));
    REQUIRE(firstMessages == second.getMessages());
}

TEST_CASE("after removeLogSink returns, the removed sink gets nothing and can be destroyed", "[base][log][sink][threads]") {
    // Keeps the loggers' records off stderr once the removed sink is gone.
    CountingSink anchor;
    const SinkRegistration anchorRegistration{anchor};
    auto removed = std::make_unique<CountingSink>();
    SinkRegistration removedRegistration{*removed};

    std::vector<std::jthread> loggers;
    for (int thread = 0; thread < LOGGING_THREADS; thread++) {
        loggers.emplace_back([](const std::stop_token stop) {
            while (!stop.stop_requested()) {
                TE_LOGGER_WARN("busy");
            }
        });
    }

    REQUIRE(waitUntil([&removed] {
        return removed->getCount() > 0;
    }));
    REQUIRE(removedRegistration.remove() == std::error_code{});
    const std::size_t countAtRemoval = removed->getCount();

    const std::size_t anchorAtRemoval = anchor.getCount();
    REQUIRE(waitUntil([&anchor, anchorAtRemoval] {
        return anchor.getCount() > anchorAtRemoval + RECORDS_AFTER_REMOVAL;
    }));
    REQUIRE(removed->getCount() == countAtRemoval);

    // Nothing is asserted after the reset: a late write into the freed sink is ASan's and
    // TSan's to report.
    removed.reset();
    const std::size_t anchorAtDestruction = anchor.getCount();
    REQUIRE(waitUntil([&anchor, anchorAtDestruction] {
        return anchor.getCount() > anchorAtDestruction + RECORDS_AFTER_REMOVAL;
    }));
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

// No capture guard: an empty sink set is the whole point. initLogging() is never called from the
// suite, so nothing else is installed either.
TEST_CASE("with no sink installed a record still reaches stderr", "[base][log][sink]") {
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const std::string captured = captureStderr([] {
        TE_LOGGER_ERROR("fallback {0}", 7);
    });

    TechEngine::setMinLevel(previous);

    REQUIRE(captured.find("fallback 7") != std::string::npos);
    REQUIRE(captured.find("LogTests.cpp") != std::string::npos);
}

class ReentrantSink final : public TechEngine::LogSink {
public:
    void write(const TechEngine::LogRecord& record) noexcept override {
        if (record.message == "outer") {
            TE_LOGGER_WARN("inner {0}", 1);
        }
    }
};

TEST_CASE("a record logged inside a write reaches the ring and stderr, not the sinks", "[base][log][sink]") {
    TechEngine::ringClear();
    const TechEngineTests::LogCaptureGuard capture;
    ReentrantSink reentrant;
    const SinkRegistration registration{reentrant};

    const std::string captured = captureStderr([] {
        TE_LOGGER_WARN("outer");
    });

    REQUIRE(captured.find("inner 1") != std::string::npos);
    REQUIRE(capture.getMessages() == std::vector<std::string>{"outer"});

    std::vector<TechEngine::StoredLogRecord> snapshot;
    TechEngine::ringSnapshot(snapshot);
    REQUIRE(snapshot.size() == 2);
    REQUIRE(snapshot[0].message == "outer");
    REQUIRE(snapshot[1].message == "inner 1");
}

class SinkChangingSink final : public TechEngine::LogSink {
public:
    void write(const TechEngine::LogRecord&) noexcept override {
        m_writes++;
        if (m_writes == 1) {
            m_addError = TechEngine::addLogSink(m_unregistered);
            m_removeError = TechEngine::removeLogSink(*this);
        }
    }

    std::size_t getWrites() const {
        return m_writes;
    }

    std::size_t getUnregisteredWrites() const {
        return m_unregistered.getCount();
    }

    std::error_code getAddError() const {
        return m_addError;
    }

    std::error_code getRemoveError() const {
        return m_removeError;
    }

private:
    CountingSink m_unregistered;
    std::size_t m_writes = 0;
    std::error_code m_addError;
    std::error_code m_removeError;
};

TEST_CASE("adding or removing a sink inside a write returns InsideSink", "[base][log][sink]") {
    SinkChangingSink changing;
    SinkRegistration registration{changing};

    TE_LOGGER_WARN("first");
    TE_LOGGER_WARN("second");

    REQUIRE(changing.getAddError() == TechEngine::LogError::InsideSink);
    REQUIRE(changing.getRemoveError() == TechEngine::LogError::InsideSink);
    REQUIRE(changing.getWrites() == 2);
    REQUIRE(changing.getUnregisteredWrites() == 0);
    REQUIRE(registration.remove() == std::error_code{});
}

TEST_CASE("a record carries a wall-clock stamp", "[base][log]") {
    const TechEngineTests::LogCaptureGuard guard;

    TE_LOGGER_WARN("first");
    TE_LOGGER_WARN("second");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 2);
    REQUIRE(captured[0].time.time_since_epoch().count() > 0);
    REQUIRE(captured[1].time >= captured[0].time);
}

static TechEngine::LogRecord sampleRecord(std::string_view message) {
    return TechEngine::LogRecord{
        .time = std::chrono::system_clock::now(),
        .tick = 1043,
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

    const std::size_t size = TechEngine::internal::flattenRecord(record, out.data(), out.size());
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

    const std::size_t tick = line.find("[t 1043]");
    const std::size_t channel = line.find("[default/default]");
    const std::size_t site = line.find("[renderer.cpp:88:renderScene()]");
    const std::size_t level = line.find("[INFO]");

    REQUIRE(tick != std::string::npos);
    REQUIRE(channel != std::string::npos);
    REQUIRE(site != std::string::npos);
    REQUIRE(level != std::string::npos);

    // Field order is the contract: a sink or a log grep reads positionally.
    REQUIRE(tick == 14);
    REQUIRE(tick < channel);
    REQUIRE(channel < site);
    REQUIRE(site < level);
    REQUIRE(line.ends_with("] swapchain 1920x1080"));
}

TEST_CASE("flatten marks its own truncation", "[base][log][format]") {
    std::array<char, 64> out{};
    const std::string huge(4096, 'x');
    const TechEngine::LogRecord record = sampleRecord(huge);

    const std::size_t size = TechEngine::internal::flattenRecord(record, out.data(), out.size());
    const std::string line{out.data(), size};

    REQUIRE(size <= out.size());
    REQUIRE(line.ends_with(TechEngine::internal::TRUNCATION_MARKER));
}

TEST_CASE("flatten never writes past its buffer", "[base][log][format]") {
    // One allocation, split by hand: the tail is the canary, so an overrun is observable
    // without relying on two arrays happening to be adjacent.
    std::array<char, 112> storage{};
    constexpr std::size_t capacity = 96;
    const std::string huge(4096, 'y');
    const TechEngine::LogRecord record = sampleRecord(huge);

    const std::size_t size = TechEngine::internal::flattenRecord(record, storage.data(), capacity);

    REQUIRE(size <= capacity);
    for (std::size_t i = capacity; i < storage.size(); i++) {
        REQUIRE(storage[i] == '\0');
    }
}

// Last on purpose: it exhausts the channel table, and registration is never unwound. Under
// ctest each case is its own process, so this only constrains a direct exe run.
TEST_CASE("registry overflow degrades to the default channel", "[base][log][channel]") {
    const TechEngineTests::LogCaptureGuard guard;

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
    TE_LOGGER_WARN_CH(last, "still delivered");

    const std::vector<TechEngine::StoredLogRecord> captured = guard.getRecords();
    REQUIRE(captured.size() == 1);
    REQUIRE(captured[0].message == "still delivered");
}

// The ring is always-on process-global state, not a pluggable sink; it has no guard of its
// own to add/remove, so each ring case resets it directly.
TEST_CASE("the ring captures records even with no sink installed", "[base][log][ring]") {
    TechEngine::ringClear();
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    TE_LOGGER_WARN("no sink needed");

    TechEngine::setMinLevel(previous);

    std::vector<TechEngine::StoredLogRecord> snapshot;
    TechEngine::ringSnapshot(snapshot);

    REQUIRE(snapshot.size() == 1);
    REQUIRE(snapshot[0].message == "no sink needed");
    REQUIRE(snapshot[0].level == TechEngine::Level::Warn);
}

TEST_CASE("the ring keeps the last LOG_RING_CAPACITY records, oldest evicted first", "[base][log][ring]") {
    TechEngine::ringClear();
    const TechEngine::Level previous = TechEngine::minLevel();
    TechEngine::setMinLevel(TechEngine::Level::Trace);

    const int total = static_cast<int>(TechEngine::LOG_RING_CAPACITY) + 5;
    for (int i = 0; i < total; i++) {
        TE_LOGGER_WARN("record {0}", i);
    }

    TechEngine::setMinLevel(previous);

    std::vector<TechEngine::StoredLogRecord> snapshot;
    TechEngine::ringSnapshot(snapshot);

    REQUIRE(snapshot.size() == TechEngine::LOG_RING_CAPACITY);
    REQUIRE(snapshot.front().message == "record 5");
    REQUIRE(snapshot.back().message == "record " + std::to_string(total - 1));
}

TEST_CASE("ringSnapshot appends to the records already in out", "[base][log][ring]") {
    TechEngine::ringClear();
    TE_LOGGER_WARN("one");
    TE_LOGGER_WARN("two");

    std::vector<TechEngine::StoredLogRecord> snapshot;
    TechEngine::ringSnapshot(snapshot);
    TechEngine::ringSnapshot(snapshot);

    REQUIRE(snapshot.size() == 4);
    REQUIRE(snapshot[0].message == "one");
    REQUIRE(snapshot[1].message == "two");
    REQUIRE(snapshot[2].message == "one");
    REQUIRE(snapshot[3].message == "two");
}

TEST_CASE("a ring snapshot keeps its messages after later records reuse every slot", "[base][log][ring]") {
    TechEngine::ringClear();
    TE_LOGGER_WARN("kept {0}", 1);
    TE_LOGGER_WARN("kept {0}", 2);

    std::vector<TechEngine::StoredLogRecord> snapshot;
    TechEngine::ringSnapshot(snapshot);
    for (std::size_t i = 0; i < TechEngine::LOG_RING_CAPACITY; i++) {
        TE_LOGGER_WARN("overwrite {0}", i);
    }

    REQUIRE(snapshot.size() == 2);
    REQUIRE(snapshot[0].message == "kept 1");
    REQUIRE(snapshot[1].message == "kept 2");
}

TEST_CASE("a ring snapshot taken while another thread logs reads every message whole", "[base][log][ring]") {
    TechEngine::ringClear();
    CountingSink quiet;
    REQUIRE(TechEngine::addLogSink(quiet) == std::error_code{});
    std::atomic<bool> stop = false;
    std::atomic<std::size_t> logged = 0;
    std::thread logger([&stop, &logged] {
        while (!stop.load()) {
            TE_LOGGER_WARN("worker record {0}", logged.load());
            logged++;
        }
    });
    while (logged.load() < TechEngine::LOG_RING_CAPACITY) {
        std::this_thread::yield();
    }

    for (int round = 0; round < 200; round++) {
        std::vector<TechEngine::StoredLogRecord> snapshot;
        TechEngine::ringSnapshot(snapshot);
        for (const TechEngine::StoredLogRecord& record: snapshot) {
            CHECK(record.message.starts_with("worker record "));
        }
    }
    stop.store(true);
    logger.join();
    CHECK(TechEngine::removeLogSink(quiet) == std::error_code{});
}