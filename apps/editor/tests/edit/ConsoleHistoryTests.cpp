#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <edit/ConsoleHistory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using TechEngine::ConsoleHistory;
using TechEngine::Level;
using TechEngine::LogRecord;
using TechEngine::StoredLogRecord;

static StoredLogRecord storedAt(const Level level, const std::string_view message) {
    LogRecord record;
    record.time = std::chrono::system_clock::now();
    record.level = level;
    record.message = message;
    record.file = "ConsoleHistoryTests.cpp";
    record.function = "storedAt";
    record.line = 1;
    return StoredLogRecord(record);
}

static void addOne(ConsoleHistory& history, const Level level, const std::string_view message) {
    std::vector<StoredLogRecord> batch;
    batch.push_back(storedAt(level, message));
    history.add(std::move(batch), 0);
}

static void addNumbered(ConsoleHistory& history, const Level level, const std::size_t first, const std::size_t count) {
    std::vector<StoredLogRecord> batch;
    for (std::size_t i = first; i < first + count; i++) {
        batch.push_back(storedAt(level, "record " + std::to_string(i)));
    }
    history.add(std::move(batch), 0);
}

static std::vector<std::string> visibleMessages(const ConsoleHistory& history) {
    std::vector<std::string> messages;
    for (std::size_t row = 0; row < history.getVisibleCount(); row++) {
        messages.push_back(history.getVisibleRecord(row).message);
    }
    return messages;
}

TEST_CASE("the history keeps 5,000 records and drops the oldest first", "[editor][edit][console]") {
    ConsoleHistory history;

    SECTION("across batches") {
        addNumbered(history, Level::Info, 0, ConsoleHistory::RECORD_CAPACITY);
        REQUIRE(history.getRecordCount() == ConsoleHistory::RECORD_CAPACITY);
        addNumbered(history, Level::Info, ConsoleHistory::RECORD_CAPACITY, 3);
    }
    SECTION("inside one batch larger than the bound") {
        addNumbered(history, Level::Info, 0, ConsoleHistory::RECORD_CAPACITY + 3);
    }

    CHECK(history.getRecordCount() == ConsoleHistory::RECORD_CAPACITY);
    REQUIRE(history.getVisibleCount() == ConsoleHistory::RECORD_CAPACITY);
    CHECK(history.getVisibleRecord(0).message == "record 3");
    CHECK(history.getVisibleRecord(ConsoleHistory::RECORD_CAPACITY - 1).message == "record 5002");
}

TEST_CASE("the level counts follow adds, drops and Clear", "[editor][edit][console]") {
    ConsoleHistory history;
    addOne(history, Level::Info, "first info");
    addOne(history, Level::Info, "second info");
    addOne(history, Level::Warn, "a warn");
    addOne(history, Level::Error, "an error");

    CHECK(history.getCount(Level::Info) == 2);
    CHECK(history.getCount(Level::Warn) == 1);
    CHECK(history.getCount(Level::Error) == 1);
    CHECK(history.getCount(Level::Debug) == 0);

    addNumbered(history, Level::Debug, 0, ConsoleHistory::RECORD_CAPACITY - 2);
    CHECK(history.getCount(Level::Info) == 0);
    CHECK(history.getCount(Level::Warn) == 1);
    CHECK(history.getCount(Level::Error) == 1);
    CHECK(history.getCount(Level::Debug) == ConsoleHistory::RECORD_CAPACITY - 2);

    history.clear();
    CHECK(history.getRecordCount() == 0);
    CHECK(history.getVisibleCount() == 0);
    for (const Level level: {Level::Trace, Level::Debug, Level::Info, Level::Warn, Level::Error, Level::Critical}) {
        CHECK(history.getCount(level) == 0);
    }

    addOne(history, Level::Warn, "after clear");
    CHECK(history.getCount(Level::Warn) == 1);
    CHECK(visibleMessages(history) == std::vector<std::string>{"after clear"});
}

TEST_CASE("a level toggle hides that level's rows and keeps its count", "[editor][edit][console]") {
    ConsoleHistory history;
    addOne(history, Level::Info, "a");
    addOne(history, Level::Warn, "b");
    addOne(history, Level::Error, "c");

    history.setLevelShown(Level::Warn, false);
    CHECK_FALSE(history.isLevelShown(Level::Warn));
    CHECK(visibleMessages(history) == std::vector<std::string>{"a", "c"});
    CHECK(history.getCount(Level::Warn) == 1);

    addOne(history, Level::Warn, "d");
    CHECK(visibleMessages(history) == std::vector<std::string>{"a", "c"});
    CHECK(history.getCount(Level::Warn) == 2);

    history.setLevelShown(Level::Warn, true);
    CHECK(visibleMessages(history) == std::vector<std::string>{"a", "b", "c", "d"});
}

TEST_CASE("the search hides rows whose message does not contain it, ignoring case", "[editor][edit][console]") {
    ConsoleHistory history;
    addOne(history, Level::Info, "Loaded Scene");
    addOne(history, Level::Warn, "saved scene");
    addOne(history, Level::Info, "Indexed 4 assets");

    history.setSearch("SCENE");
    CHECK(history.getSearch() == "SCENE");
    CHECK(visibleMessages(history) == std::vector<std::string>{"Loaded Scene", "saved scene"});

    SECTION("only the message is searched") {
        history.setSearch("storedAt");
        CHECK(history.getVisibleCount() == 0);
    }
    SECTION("a record added later is filtered as it arrives") {
        addOne(history, Level::Info, "Startup.tescene read");
        addOne(history, Level::Info, "window opened");
        CHECK(visibleMessages(history) == std::vector<std::string>{"Loaded Scene", "saved scene", "Startup.tescene read"});
    }
    SECTION("the search and a level toggle combine") {
        history.setLevelShown(Level::Info, false);
        CHECK(visibleMessages(history) == std::vector<std::string>{"saved scene"});
    }
    SECTION("an empty search shows every row again") {
        history.setSearch("");
        CHECK(history.getVisibleCount() == 3);
    }
}

TEST_CASE("the bound drops the oldest record while a filter hides rows", "[editor][edit][console]") {
    ConsoleHistory history;
    addOne(history, Level::Warn, "oldest warn");
    addNumbered(history, Level::Info, 0, ConsoleHistory::RECORD_CAPACITY - 1);
    history.setLevelShown(Level::Info, false);
    REQUIRE(visibleMessages(history) == std::vector<std::string>{"oldest warn"});

    addOne(history, Level::Warn, "newest warn");

    CHECK(history.getRecordCount() == ConsoleHistory::RECORD_CAPACITY);
    CHECK(history.getCount(Level::Warn) == 1);
    CHECK(visibleMessages(history) == std::vector<std::string>{"newest warn"});

    history.setLevelShown(Level::Info, true);
    REQUIRE(history.getVisibleCount() == ConsoleHistory::RECORD_CAPACITY);
    CHECK(history.getVisibleRecord(0).message == "record 0");
}

TEST_CASE("a drain that reports dropped records adds one \"N records dropped\" row before its batch", "[editor][edit][console]") {
    ConsoleHistory history;
    addOne(history, Level::Info, "before");

    std::vector<StoredLogRecord> batch;
    batch.push_back(storedAt(Level::Info, "kept 1"));
    batch.push_back(storedAt(Level::Info, "kept 2"));
    history.add(std::move(batch), 3);

    CHECK(history.getRecordCount() == 4);
    CHECK(visibleMessages(history) == std::vector<std::string>{"before", "3 records dropped", "kept 1", "kept 2"});

    SECTION("a drain that dropped nothing adds no row") {
        addOne(history, Level::Info, "after");
        CHECK(history.getRecordCount() == 5);
    }
    SECTION("a drop with an empty batch still adds its row") {
        history.add({}, 7);
        CHECK(visibleMessages(history).back() == "7 records dropped");
    }
}
