#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/process/Process.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <system_error>
#include <thread>

using TechEngine::FileError;
using TechEngine::Process;
using TechEngineTests::ScratchDirectory;

static const std::filesystem::path STUB_PATH = TE_PROCESS_STUB_PATH;
static constexpr int STUB_EXIT_CODE = 42;
static constexpr const char* STUB_MARKER = "ProcessStubRan.txt";

static bool waitForExit(Process& process, int& exitCode) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < deadline) {
        if (process.pollExit(exitCode)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

TEST_CASE("Process runs a program in its working directory and reports its exit code", "[platform][process]") {
    const ScratchDirectory scratch{"processRuns"};
    Process process;

    const std::error_code startError = process.start(STUB_PATH, scratch.root());
    REQUIRE_FALSE(startError);
    CHECK(process.isRunning());

    int exitCode = -1;
    REQUIRE(waitForExit(process, exitCode));
    CHECK(exitCode == STUB_EXIT_CODE);
    CHECK_FALSE(process.isRunning());
    CHECK(std::filesystem::exists(scratch.root() / STUB_MARKER));

    int laterExitCode = -1;
    CHECK_FALSE(process.pollExit(laterExitCode));
    CHECK(laterExitCode == -1);
}

TEST_CASE("Process starts again after its child exits", "[platform][process]") {
    const ScratchDirectory first{"processFirstRun"};
    const ScratchDirectory second{"processSecondRun"};
    Process process;

    const std::error_code firstError = process.start(STUB_PATH, first.root());
    REQUIRE_FALSE(firstError);
    int firstExitCode = -1;
    REQUIRE(waitForExit(process, firstExitCode));

    const std::error_code secondError = process.start(STUB_PATH, second.root());
    REQUIRE_FALSE(secondError);
    CHECK(process.isRunning());
    int secondExitCode = -1;
    REQUIRE(waitForExit(process, secondExitCode));

    CHECK(firstExitCode == STUB_EXIT_CODE);
    CHECK(secondExitCode == STUB_EXIT_CODE);
    CHECK(std::filesystem::exists(second.root() / STUB_MARKER));
}

TEST_CASE("Process resolves a relative executable against the caller's directory, not the child's", "[platform][process]") {
    const ScratchDirectory scratch{"processRelative"};
    const std::filesystem::path relative = std::filesystem::relative(STUB_PATH);
    REQUIRE(relative.is_relative());
    Process process;

    const std::error_code startError = process.start(relative, scratch.root());
    REQUIRE_FALSE(startError);
    int exitCode = -1;
    REQUIRE(waitForExit(process, exitCode));

    CHECK(exitCode == STUB_EXIT_CODE);
    CHECK(std::filesystem::exists(scratch.root() / STUB_MARKER));
}

TEST_CASE("Process refuses paths it cannot run and starts nothing", "[platform][process]") {
    const ScratchDirectory scratch{"processRefuses"};
    Process process;
    std::error_code startError;

    SECTION("a missing executable") {
        startError = process.start(scratch.root() / "missing", scratch.root());
        CHECK(startError == FileError::NotFound);
    }
    SECTION("a folder as the executable") {
        startError = process.start(scratch.makeDirectory("folder"), scratch.root());
        CHECK(startError == FileError::IsADirectory);
    }
    SECTION("a missing working directory") {
        startError = process.start(STUB_PATH, scratch.root() / "missing");
        CHECK(startError == FileError::NotFound);
    }
    SECTION("a file as the working directory") {
        startError = process.start(STUB_PATH, scratch.writeFile("file.txt", "not a folder"));
        CHECK(startError == FileError::NotADirectory);
    }
#if !defined(_WIN32)
    SECTION("a file without an execute bit") {
        const std::filesystem::path script = scratch.writeFile("script", "#!/bin/sh\nexit 0\n");
        std::filesystem::permissions(script, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
        startError = process.start(script, scratch.root());
        CHECK(startError == FileError::AccessDenied);
    }
    SECTION("an executable file in no format the kernel runs") {
        const std::filesystem::path garbage = scratch.writeFile("garbage", "not a program");
        std::filesystem::permissions(garbage, std::filesystem::perms::owner_all);
        startError = process.start(garbage, scratch.root());
        CHECK(startError == FileError::AccessDenied);
    }
    SECTION("a script whose interpreter is missing") {
        const std::filesystem::path script = scratch.writeFile("script", "#!/missing/interpreter\n");
        std::filesystem::permissions(script, std::filesystem::perms::owner_all);
        startError = process.start(script, scratch.root());
        CHECK(startError == FileError::NotFound);
    }
#endif

    CHECK_FALSE(process.isRunning());
    int exitCode = -1;
    CHECK_FALSE(process.pollExit(exitCode));
    CHECK_FALSE(std::filesystem::exists(scratch.root() / STUB_MARKER));

    const std::error_code retryError = process.start(STUB_PATH, scratch.root());
    REQUIRE_FALSE(retryError);
    REQUIRE(waitForExit(process, exitCode));
    CHECK(exitCode == STUB_EXIT_CODE);
}

#if !defined(_WIN32)
TEST_CASE("Process reports a child a signal killed as 128 plus the signal number", "[platform][process]") {
    const ScratchDirectory scratch{"processSignalled"};
    const std::filesystem::path script = scratch.writeFile("killsItself", "#!/bin/sh\nkill -9 $$\n");
    std::filesystem::permissions(script, std::filesystem::perms::owner_all);
    Process process;

    const std::error_code startError = process.start(script, scratch.root());
    REQUIRE_FALSE(startError);
    int exitCode = -1;
    REQUIRE(waitForExit(process, exitCode));

    CHECK(exitCode == 128 + 9);
    CHECK_FALSE(process.isRunning());
}
#endif
