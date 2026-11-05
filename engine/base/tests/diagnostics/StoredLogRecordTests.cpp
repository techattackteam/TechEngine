#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>

using TechEngine::Level;
using TechEngine::LogChannel;
using TechEngine::LogModule;
using TechEngine::LogRecord;
using TechEngine::StoredLogRecord;

TEST_CASE("a stored record copies every field of the record it is made from", "[base][log][stored]") {
    const std::string message = "stored message";
    LogRecord record;
    record.time = std::chrono::system_clock::time_point{std::chrono::seconds{42}};
    record.tick = 1043;
    record.level = Level::Error;
    record.moduleTag = LogModule{3};
    record.channel = LogChannel{7};
    record.message = message;
    record.file = "renderer.cpp";
    record.function = "renderScene";
    record.line = 88;

    const StoredLogRecord stored(record);

    CHECK(stored.time == record.time);
    CHECK(stored.tick == 1043);
    CHECK(stored.level == Level::Error);
    CHECK(stored.moduleTag == LogModule{3});
    CHECK(stored.channel == LogChannel{7});
    CHECK(stored.message == "stored message");
    CHECK(stored.file == "renderer.cpp");
    CHECK(stored.function == "renderScene");
    CHECK(stored.line == 88);
}

TEST_CASE("a stored record keeps its message after the buffer it was logged from changes", "[base][log][stored]") {
    std::string buffer = "the original message";
    LogRecord record;
    record.message = buffer;

    const StoredLogRecord stored(record);
    buffer.assign(buffer.size(), 'x');

    CHECK(stored.message == "the original message");
    CHECK(stored.message.data() != buffer.data());
}
