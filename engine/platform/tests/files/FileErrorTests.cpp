#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <system_error>

using TechEngine::FileError;
using TechEngine::fileErrorCategory;

static constexpr std::array<FileError, 9> ALL_CODES{FileError::InvalidPath, FileError::NoMount, FileError::NotFound, FileError::IsADirectory, FileError::NotADirectory, FileError::AlreadyExists, FileError::NotEmpty, FileError::AccessDenied, FileError::IoError};

TEST_CASE("every FileError code converts into the file category", "[files][fileerror]") {
    TechEngineTests::checkErrorCategory<FileError>(ALL_CODES, fileErrorCategory(), "file");
}

TEST_CASE("a default error_code is success and matches no FileError", "[files][fileerror]") {
    const std::error_code success{};

    CHECK_FALSE(success);
    for (const FileError code: ALL_CODES) {
        CHECK_FALSE(success == code);
    }
}
