#include <TechEngine/platform/Platform.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using TechEngine::executablePath;

TEST_CASE("executablePath names a file that exists", "[platform][executablepath]") {
    const std::filesystem::path& path = executablePath();

    CHECK_FALSE(path.empty());
    CHECK(path.is_absolute());
    CHECK(std::filesystem::exists(path));
    CHECK(std::filesystem::is_regular_file(path));
}

TEST_CASE("executablePath names the running test binary", "[platform][executablepath]") {
    const std::filesystem::path stem = executablePath().stem();

    CHECK(stem == "TechEnginePlatformTests");
}

TEST_CASE("executablePath does not depend on the working directory", "[platform][executablepath]") {
    const std::filesystem::path first = executablePath();

    const std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(previous.root_path());
    const std::filesystem::path second = executablePath();
    std::filesystem::current_path(previous);

    CHECK(first == second);
}
