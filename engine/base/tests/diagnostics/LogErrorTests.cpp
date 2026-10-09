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
using TechEngine::LogSinkFn;

static constexpr std::array<LogError, 3> ALL_CODES{LogError::NullSink, LogError::SinkTableFull, LogError::SinkNotRegistered};

static std::array<std::size_t, 9> g_fillerCalls{};

template<std::size_t N>
static void fillerSink(const TechEngine::LogRecord&) {
    g_fillerCalls[N]++;
}

static constexpr std::array<LogSinkFn, 9> FILLER_SINKS{&fillerSink<0>, &fillerSink<1>, &fillerSink<2>, &fillerSink<3>, &fillerSink<4>, &fillerSink<5>, &fillerSink<6>, &fillerSink<7>, &fillerSink<8>};

// The sink table is process-global and Catch2 shares one process, so every sink a case adds
// has to come back out even when a REQUIRE stops the case early.
class FillerGuard {
private:
    std::vector<LogSinkFn> m_added;

public:
    FillerGuard() = default;

    ~FillerGuard() {
        for (const LogSinkFn sink: m_added) {
            TechEngine::removeLogSink(sink);
        }
    }

    FillerGuard(const FillerGuard&) = delete;

    FillerGuard& operator=(const FillerGuard&) = delete;

    void added(const LogSinkFn sink) {
        m_added.push_back(sink);
    }
};

TEST_CASE("every LogError code converts into the log category", "[base][log][logerror]") {
    TechEngineTests::checkErrorCategory<LogError>(ALL_CODES, logErrorCategory(), "log");
}

TEST_CASE("addLogSink rejects a null sink", "[base][log][logerror]") {
    CHECK(TechEngine::addLogSink(nullptr) == LogError::NullSink);
}

TEST_CASE("removeLogSink reports a sink that was never registered", "[base][log][logerror]") {
    CHECK(TechEngine::removeLogSink(&fillerSink<8>) == LogError::SinkNotRegistered);
}

TEST_CASE("addLogSink reports a full table and leaves the rejected sink out", "[base][log][logerror]") {
    FillerGuard guard;
    LogSinkFn rejected = nullptr;

    for (const LogSinkFn sink: FILLER_SINKS) {
        const std::error_code addError = TechEngine::addLogSink(sink);
        if (addError) {
            CHECK(addError == LogError::SinkTableFull);
            rejected = sink;
            break;
        }
        guard.added(sink);
    }

    REQUIRE(rejected != nullptr);
    CHECK(TechEngine::removeLogSink(rejected) == LogError::SinkNotRegistered);
}
