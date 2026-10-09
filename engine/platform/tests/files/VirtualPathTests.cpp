#include <TechEngine/platform/files/VirtualPath.hpp>

#include <catch2/catch_test_macros.hpp>

#include <system_error>

using TechEngine::FileError;
using TechEngine::splitVirtualPath;
using TechEngine::VirtualPathParts;

TEST_CASE("splitVirtualPath separates the alias from the relative path", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://ui/icon.png", parts) == std::error_code{});
    CHECK(parts.alias == "editorAssets");
    CHECK(parts.relative == "ui/icon.png");
}

TEST_CASE("splitVirtualPath accepts an alias-only path", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://", parts) == std::error_code{});
    CHECK(parts.alias == "editorAssets");
    CHECK(parts.relative.empty());
}

TEST_CASE("splitVirtualPath preserves case on both sides", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("EditorAssets://UI/Icon.PNG", parts) == std::error_code{});
    CHECK(parts.alias == "EditorAssets");
    CHECK(parts.relative == "UI/Icon.PNG");
}

TEST_CASE("splitVirtualPath rejects a path with no scheme separator", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK(splitVirtualPath("ui/icon.png", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets:/ui/icon.png", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("", parts) == FileError::InvalidPath);
}

TEST_CASE("splitVirtualPath rejects a malformed alias", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK(splitVirtualPath("://ui/icon.png", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editor/assets://ui/icon.png", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editor:assets://ui/icon.png", parts) == FileError::InvalidPath);
}

TEST_CASE("splitVirtualPath rejects a relative that would escape the mount root", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK(splitVirtualPath("editorAssets://../secrets.txt", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets://ui/../../secrets.txt", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets://ui/..", parts) == FileError::InvalidPath);
}

TEST_CASE("splitVirtualPath rejects a relative that is absolute or uses backslashes", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK(splitVirtualPath("a:///etc/passwd", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets:///etc/passwd", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets://C:/Windows/win.ini", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets://C:Windows/win.ini", parts) == FileError::InvalidPath);
    CHECK(splitVirtualPath("editorAssets://ui\\icon.png", parts) == FileError::InvalidPath);
}

TEST_CASE("splitVirtualPath does not over-reject dots inside a name", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://ui/..hidden/icon..png", parts) == std::error_code{});
    CHECK(parts.relative == "ui/..hidden/icon..png");
}
