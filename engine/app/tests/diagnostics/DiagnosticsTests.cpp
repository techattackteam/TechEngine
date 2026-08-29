#include <TechEngine/base/diagnostics/Log.hpp>

#include <diagnostics/Diagnostics.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

static std::string readLogFile(const std::filesystem::path& path) {
    std::ifstream stream(path);
    return std::string{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

// GOTCHA: the only case in the engine that installs the spdlog sink, and so the only writer of
// logs/techengine.log — a path relative to the working directory ctest gives this exe, which is
// this module's binary dir. A second case writing it would race this one under `ctest -j`.
//
// The second half is what proves the destructor: the file is truncated on open (ADR-011 §3), so
// if shutdownLogging() had not run, the second scope's init would be a no-op and the first
// line would survive. Error level, because the compile-time gate keeps it in every config.
TEST_CASE("the diagnostics scope opens and closes the session log file", "[app][diagnostics]") {
    const std::filesystem::path logFile = "logs/techengine.log";
    std::filesystem::remove(logFile);

    {
        const TechEngine::DiagnosticsScope diagnostics;
        TE_LOGGER_ERROR("first scope reached the file sink");
    }

    REQUIRE(std::filesystem::exists(logFile));
    REQUIRE(readLogFile(logFile).find("first scope reached the file sink") != std::string::npos);

    {
        const TechEngine::DiagnosticsScope diagnostics;
        TE_LOGGER_ERROR("second scope reached the file sink");
    }

    const std::string contents = readLogFile(logFile);

    REQUIRE(contents.find("second scope reached the file sink") != std::string::npos);
    REQUIRE(contents.find("first scope reached the file sink") == std::string::npos);
}
