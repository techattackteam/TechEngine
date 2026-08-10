#include <TechEngine/platform/files/VirtualPath.hpp>

#include <catch2/catch_test_macros.hpp>

using TechEngine::splitVirtualPath;
using TechEngine::VirtualPathParts;

TEST_CASE("splitVirtualPath separates the alias from the relative path", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://ui/icon.png", parts));
    CHECK(parts.alias == "editorAssets");
    CHECK(parts.relative == "ui/icon.png");
}

TEST_CASE("splitVirtualPath accepts an alias-only path", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://", parts));
    CHECK(parts.alias == "editorAssets");
    CHECK(parts.relative.empty());
}

TEST_CASE("splitVirtualPath preserves case on both sides", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("EditorAssets://UI/Icon.PNG", parts));
    CHECK(parts.alias == "EditorAssets");
    CHECK(parts.relative == "UI/Icon.PNG");
}

TEST_CASE("splitVirtualPath rejects a path with no scheme separator", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK_FALSE(splitVirtualPath("ui/icon.png", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets:/ui/icon.png", parts));
    CHECK_FALSE(splitVirtualPath("", parts));
}

TEST_CASE("splitVirtualPath rejects a malformed alias", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK_FALSE(splitVirtualPath("://ui/icon.png", parts));
    CHECK_FALSE(splitVirtualPath("editor/assets://ui/icon.png", parts));
    CHECK_FALSE(splitVirtualPath("editor:assets://ui/icon.png", parts));
}

TEST_CASE("splitVirtualPath rejects a relative that would escape the mount root", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK_FALSE(splitVirtualPath("editorAssets://../secrets.txt", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets://ui/../../secrets.txt", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets://ui/..", parts));
}

TEST_CASE("splitVirtualPath rejects a relative that is absolute or uses backslashes", "[files][virtualpath]") {
    VirtualPathParts parts;

    CHECK_FALSE(splitVirtualPath("a:///etc/passwd", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets:///etc/passwd", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets://C:/Windows/win.ini", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets://C:Windows/win.ini", parts));
    CHECK_FALSE(splitVirtualPath("editorAssets://ui\\icon.png", parts));
}

TEST_CASE("splitVirtualPath does not over-reject dots inside a name", "[files][virtualpath]") {
    VirtualPathParts parts;

    REQUIRE(splitVirtualPath("editorAssets://ui/..hidden/icon..png", parts));
    CHECK(parts.relative == "ui/..hidden/icon..png");
}
