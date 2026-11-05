#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <edit/ConsoleLog.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::ConsoleLog;
using TechEngine::Level;
using TechEngine::LogRecord;
using TechEngine::StoredLogRecord;

static LogRecord recordAt(const Level level, const std::string_view message) {
    LogRecord record;
    record.time = std::chrono::system_clock::now();
    record.level = level;
    record.message = message;
    record.file = "ConsoleLogTests.cpp";
    record.function = "recordAt";
    record.line = 1;
    return record;
}

static void writeFromAnotherThread(ConsoleLog& console, const Level level, const std::string_view message) {
    std::thread writer([&console, level, message] {
        const std::string owned(message);
        console.write(recordAt(level, owned));
    });
    writer.join();
}

TEST_CASE("a record logged on another thread reaches the drain with its message after the log call returns", "[editor][edit][console]") {
    TechEngine::ringClear();
    ConsoleLog console([] {
    });
    REQUIRE(console.start() == std::error_code{});

    std::thread logger([] {
        TE_LOGGER_WARN("console message {0} from {1}", 42, "a worker");
    });
    logger.join();

    std::vector<StoredLogRecord> drained;
    console.drain(drained);
    CHECK(console.stop() == std::error_code{});

    REQUIRE(drained.size() == 1);
    CHECK(drained[0].message == "console message 42 from a worker");
    CHECK(drained[0].level == Level::Warn);
    CHECK(drained[0].file == "ConsoleLogTests.cpp");
    CHECK_FALSE(drained[0].function.empty());
    CHECK(drained[0].line > 0);
}

TEST_CASE("a full queue keeps every record and reports no drop", "[editor][edit][console]") {
    ConsoleLog console([] {
    });
    for (std::size_t i = 0; i < ConsoleLog::QUEUE_CAPACITY; i++) {
        const std::string message = std::to_string(i);
        console.write(recordAt(Level::Info, message));
    }

    std::vector<StoredLogRecord> drained;
    const std::size_t dropped = console.drain(drained);

    CHECK(dropped == 0);
    REQUIRE(drained.size() == ConsoleLog::QUEUE_CAPACITY);
    CHECK(drained.front().message == "0");
    CHECK(drained.back().message == std::to_string(ConsoleLog::QUEUE_CAPACITY - 1));
}

TEST_CASE("an overflow drops the oldest records and the next drain reports how many, once", "[editor][edit][console]") {
    ConsoleLog console([] {
    });
    const std::size_t extra = 5;
    for (std::size_t i = 0; i < ConsoleLog::QUEUE_CAPACITY + extra; i++) {
        const std::string message = std::to_string(i);
        console.write(recordAt(Level::Info, message));
    }

    std::vector<StoredLogRecord> drained;
    const std::size_t dropped = console.drain(drained);

    CHECK(dropped == extra);
    REQUIRE(drained.size() == ConsoleLog::QUEUE_CAPACITY);
    CHECK(drained.front().message == std::to_string(extra));
    CHECK(drained.back().message == std::to_string(ConsoleLog::QUEUE_CAPACITY + extra - 1));

    std::vector<StoredLogRecord> second;
    const std::size_t droppedAgain = console.drain(second);
    CHECK(droppedAgain == 0);
    CHECK(second.empty());
}

TEST_CASE("a Warn or worse from another thread wakes main once until main drains", "[editor][edit][console]") {
    TechEngine::ringClear();
    std::atomic<int> wakes = 0;
    ConsoleLog console([&wakes] {
        wakes++;
    });
    REQUIRE(console.start() == std::error_code{});

    writeFromAnotherThread(console, Level::Warn, "first warn");
    writeFromAnotherThread(console, Level::Error, "an error before the drain");
    writeFromAnotherThread(console, Level::Critical, "a critical before the drain");
    CHECK(wakes.load() == 1);

    std::vector<StoredLogRecord> drained;
    console.drain(drained);
    CHECK(drained.size() == 3);

    writeFromAnotherThread(console, Level::Error, "an error after the drain");
    CHECK(wakes.load() == 2);

    CHECK(console.stop() == std::error_code{});
}

TEST_CASE("an Info or below from another thread, and a Warn from main, queue without waking main", "[editor][edit][console]") {
    TechEngine::ringClear();
    std::atomic<int> wakes = 0;
    ConsoleLog console([&wakes] {
        wakes++;
    });
    REQUIRE(console.start() == std::error_code{});

    writeFromAnotherThread(console, Level::Trace, "a trace from a worker");
    writeFromAnotherThread(console, Level::Debug, "a debug from a worker");
    writeFromAnotherThread(console, Level::Info, "an info from a worker");
    console.write(recordAt(Level::Warn, "a warn from main"));
    console.write(recordAt(Level::Critical, "a critical from main"));
    CHECK(wakes.load() == 0);

    std::vector<StoredLogRecord> drained;
    console.drain(drained);
    CHECK(drained.size() == 5);

    CHECK(console.stop() == std::error_code{});
}

TEST_CASE("the ring's records come before the first record the sink sees", "[editor][edit][console]") {
    TechEngine::ringClear();
    TE_LOGGER_WARN("console ring record");
    ConsoleLog console([] {
    });
    REQUIRE(console.start() == std::error_code{});
    TE_LOGGER_WARN("console live record");

    std::vector<StoredLogRecord> drained;
    console.drain(drained);
    CHECK(console.stop() == std::error_code{});

    REQUIRE(drained.size() == 2);
    CHECK(drained[0].message == "console ring record");
    CHECK(drained[0].level == Level::Warn);
    CHECK(drained[1].message == "console live record");
}

TEST_CASE("nothing reaches the queue after stop", "[editor][edit][console]") {
    TechEngine::ringClear();
    ConsoleLog console([] {
    });
    REQUIRE(console.start() == std::error_code{});
    TE_LOGGER_WARN("console before removal");
    REQUIRE(console.stop() == std::error_code{});

    TE_LOGGER_WARN("console after removal");
    std::thread logger([] {
        TE_LOGGER_WARN("console after removal from a worker");
    });
    logger.join();

    std::vector<StoredLogRecord> drained;
    console.drain(drained);
    REQUIRE(drained.size() == 1);
    CHECK(drained[0].message == "console before removal");
}

TEST_CASE("a console destroyed while registered removes itself", "[editor][edit][console]") {
    TechEngine::ringClear();
    {
        ConsoleLog console([] {
        });
        REQUIRE(console.start() == std::error_code{});
    }

    TE_LOGGER_WARN("console after its destruction");

    ConsoleLog next([] {
    });
    REQUIRE(next.start() == std::error_code{});
    std::vector<StoredLogRecord> drained;
    next.drain(drained);
    CHECK(next.stop() == std::error_code{});

    REQUIRE(drained.size() == 1);
    CHECK(drained[0].message == "console after its destruction");
}
