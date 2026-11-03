#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/LogError.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <system_error>
#include <vector>

using TechEngine::LogError;
using TechEngine::logErrorCategory;
using TechEngine::LogSink;

static constexpr std::array<LogError, 4> ALL_CODES{LogError::SinkTableFull, LogError::SinkNotRegistered, LogError::SinkAlreadyRegistered, LogError::InsideSink};

static constexpr std::size_t SINK_TABLE_CAPACITY = 8;

class FillerSink final : public LogSink {
public:
    void write(const TechEngine::LogRecord&) noexcept override {
    }
};

// The sink table is process-global and Catch2 shares one process, so every sink a case adds
// has to come back out even when a REQUIRE stops the case early.
class FillerGuard {
private:
    std::vector<LogSink*> m_added;

public:
    FillerGuard() = default;

    ~FillerGuard() {
        for (LogSink* sink: m_added) {
            TechEngine::removeLogSink(*sink);
        }
    }

    FillerGuard(const FillerGuard&) = delete;

    FillerGuard& operator=(const FillerGuard&) = delete;

    void added(LogSink& sink) {
        m_added.push_back(&sink);
    }
};

TEST_CASE("every LogError code converts into the log category", "[base][log][logerror]") {
    TechEngineTests::checkErrorCategory<LogError>(ALL_CODES, logErrorCategory(), "log");
}

TEST_CASE("removeLogSink reports a sink that was never registered", "[base][log][logerror]") {
    FillerSink never;

    CHECK(TechEngine::removeLogSink(never) == LogError::SinkNotRegistered);
}

TEST_CASE("addLogSink reports a full table and leaves the rejected sink out", "[base][log][logerror]") {
    std::array<FillerSink, SINK_TABLE_CAPACITY + 1> fillers{};
    FillerGuard guard;
    LogSink* rejected = nullptr;

    for (FillerSink& sink: fillers) {
        const std::error_code addError = TechEngine::addLogSink(sink);
        if (addError) {
            CHECK(addError == LogError::SinkTableFull);
            rejected = &sink;
            break;
        }
        guard.added(sink);
    }

    REQUIRE(rejected != nullptr);
    CHECK(TechEngine::removeLogSink(*rejected) == LogError::SinkNotRegistered);
}

TEST_CASE("a second add of the same sink is refused and takes no second slot", "[base][log][logerror]") {
    FillerSink twice;
    std::array<FillerSink, SINK_TABLE_CAPACITY - 1> others{};
    FillerGuard guard;

    REQUIRE(TechEngine::addLogSink(twice) == std::error_code{});
    guard.added(twice);
    CHECK(TechEngine::addLogSink(twice) == LogError::SinkAlreadyRegistered);

    for (FillerSink& sink: others) {
        REQUIRE(TechEngine::addLogSink(sink) == std::error_code{});
        guard.added(sink);
    }

    CHECK(TechEngine::removeLogSink(twice) == std::error_code{});
    CHECK(TechEngine::removeLogSink(twice) == LogError::SinkNotRegistered);
}

TEST_CASE("a removed sink can be added again", "[base][log][logerror]") {
    FillerSink sink;
    FillerGuard guard;

    REQUIRE(TechEngine::addLogSink(sink) == std::error_code{});
    REQUIRE(TechEngine::removeLogSink(sink) == std::error_code{});

    CHECK(TechEngine::addLogSink(sink) == std::error_code{});
    guard.added(sink);
}
