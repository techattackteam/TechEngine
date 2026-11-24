#include <edit/AssetDragPayload.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

using TechEngine::readAssetDragPayload;
using TechEngine::writeAssetDragPayload;

static std::vector<std::string> roundTrip(const std::vector<std::string>& paths) {
    std::string bytes;
    writeAssetDragPayload(paths, bytes);
    std::vector<std::string> out;
    REQUIRE(readAssetDragPayload(bytes, out));
    return out;
}

TEST_CASE("a drag of one path reads back as that path", "[editor][edit][browser]") {
    const std::vector<std::string> paths = {"project://assets/client/Kart/Kart.temodel"};

    CHECK(roundTrip(paths) == paths);
}

TEST_CASE("a drag of several paths reads back in the order written, repeats included", "[editor][edit][browser]") {
    const std::vector<std::string> paths = {
        "project://assets/client/Front/Front.temesh",
        "project://assets/client/Kart/Kart.temodel",
        "project://assets/client/Front/Front.temesh",
    };

    CHECK(roundTrip(paths) == paths);
}

TEST_CASE("a path with spaces, dots and characters past ASCII reads back unchanged", "[editor][edit][browser]") {
    const std::vector<std::string> paths = {"project://assets/client/My Kart. v2/Kart\xC3\xA9.temodel", "x"};

    CHECK(roundTrip(paths) == paths);
}

TEST_CASE("a written payload ends each path in a NUL", "[editor][edit][browser]") {
    const std::vector<std::string> paths = {"a/b", "c"};
    std::string bytes = "stale";

    writeAssetDragPayload(paths, bytes);

    CHECK(bytes == std::string("a/b\0c\0", 6));
}

TEST_CASE("a payload with no path, no final NUL or an empty path reads as nothing", "[editor][edit][browser]") {
    std::string bytes;

    SECTION("no bytes") {
        bytes = "";
    }

    SECTION("a written drag of no path") {
        writeAssetDragPayload({}, bytes);
    }

    SECTION("no final NUL") {
        bytes = std::string("a/b\0c", 5);
    }

    SECTION("an empty path between two") {
        bytes = std::string("a/b\0\0c\0", 7);
    }

    SECTION("a lone NUL") {
        bytes = std::string("\0", 1);
    }

    std::vector<std::string> out = {"stale"};
    CHECK_FALSE(readAssetDragPayload(bytes, out));
    CHECK(out.empty());
}
