#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

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

// logs/techengine.log is relative to the working directory, which ctest shares between every case
// of this exe, and every App::run() writes it. Run in parallel, another case holds the file open,
// so this case moves into its own folder first, and restores the old one before any REQUIRE can
// leave it there.
//
// The second half is what proves the destructor: the file is truncated on open, so if
// shutdownLogging() had not run, the second scope's init would be a no-op and the first line
// would survive. Error level, because the compile-time gate keeps it in every config.
TEST_CASE("the diagnostics scope opens and closes the session log file", "[app][diagnostics][os]") {
    const TechEngineTests::ScratchDirectory scratch{"sessionLog"};
    const std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(scratch.root());
    const std::filesystem::path logFile = "logs/techengine.log";

    {
        const TechEngine::DiagnosticsScope diagnostics;
        TE_LOGGER_ERROR("first scope reached the file sink");
    }

    const bool firstExists = std::filesystem::exists(logFile);
    const std::string first = readLogFile(logFile);

    {
        const TechEngine::DiagnosticsScope diagnostics;
        TE_LOGGER_ERROR("second scope reached the file sink");
    }

    const std::string second = readLogFile(logFile);
    std::filesystem::current_path(previous);

    REQUIRE(firstExists);
    REQUIRE(first.find("first scope reached the file sink") != std::string::npos);
    REQUIRE(second.find("second scope reached the file sink") != std::string::npos);
    REQUIRE(second.find("first scope reached the file sink") == std::string::npos);
}
