#include <TechEngine/platform/files/FileError.hpp>

#include <assets/index/AssetNaming.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

using TechEngine::AssetNaming;
using TechEngine::FileError;

TEST_CASE("a name is valid unless a file name cannot hold it", "[editor][assets][naming]") {
    CHECK(AssetNaming::isValidName("Material"));
    CHECK(AssetNaming::isValidName("Red Brick"));
    CHECK(AssetNaming::isValidName("Old.Brick"));
    CHECK(AssetNaming::isValidName("Brick_2"));
    CHECK(AssetNaming::isValidName(".hidden"));

    CHECK_FALSE(AssetNaming::isValidName(""));
    CHECK_FALSE(AssetNaming::isValidName("Walls/Brick"));
    CHECK_FALSE(AssetNaming::isValidName("Walls\\Brick"));
    CHECK_FALSE(AssetNaming::isValidName("C:Brick"));
    CHECK_FALSE(AssetNaming::isValidName("Brick*"));
    CHECK_FALSE(AssetNaming::isValidName("Brick?"));
    CHECK_FALSE(AssetNaming::isValidName("\"Brick\""));
    CHECK_FALSE(AssetNaming::isValidName("<Brick>"));
    CHECK_FALSE(AssetNaming::isValidName("Brick|Wall"));
    CHECK_FALSE(AssetNaming::isValidName("Brick\t"));
    CHECK_FALSE(AssetNaming::isValidName(std::string("Bri\0ck", 6)));
    CHECK_FALSE(AssetNaming::isValidName("Brick."));
    CHECK_FALSE(AssetNaming::isValidName("Brick "));
    CHECK_FALSE(AssetNaming::isValidName("."));
    CHECK_FALSE(AssetNaming::isValidName(".."));
}

TEST_CASE("a stem is the last part of a path without its extension", "[editor][assets][naming]") {
    CHECK(AssetNaming::stemOf("project://assets/client/Sponza.gltf") == "Sponza");
    CHECK(AssetNaming::stemOf("project://assets/client/Old.Town.fbx") == "Old.Town");
    CHECK(AssetNaming::stemOf("project://assets/v1.2/Crate") == "Crate");
    CHECK(AssetNaming::stemOf("project://assets/client/.hidden") == ".hidden");
    CHECK(AssetNaming::stemOf("Sky.hdr") == "Sky");
    CHECK(AssetNaming::stemOf("project://assets/client/") == "");
    CHECK(AssetNaming::stemOf("") == "");
}

TEST_CASE("a path's folder is everything before its last slash", "[editor][assets][naming]") {
    CHECK(AssetNaming::folderOf("project://assets/client/Sponza.gltf") == "project://assets/client");
    CHECK(AssetNaming::folderOf("project://assets/v1.2/Crate") == "project://assets/v1.2");
    CHECK(AssetNaming::folderOf("Sky.hdr") == "");
    CHECK(AssetNaming::folderOf("") == "");
}

TEST_CASE("joining puts one slash between a folder and a name, and none after an empty folder", "[editor][assets][naming]") {
    CHECK(AssetNaming::join("project://assets/client", "Brick") == "project://assets/client/Brick");
    CHECK(AssetNaming::join("", "Brick") == "Brick");
    CHECK(AssetNaming::join(AssetNaming::folderOf("project://assets/client/Sky.hdr"), "Dusk") == "project://assets/client/Dusk");
}

TEST_CASE("a suffix of 0 adds nothing, and any other suffix is appended after an underscore", "[editor][assets][naming]") {
    CHECK(AssetNaming::withSuffix("Brick", 0) == "Brick");
    CHECK(AssetNaming::withSuffix("Brick", 1) == "Brick_1");
    CHECK(AssetNaming::withSuffix("Brick", 12) == "Brick_12");
    CHECK(AssetNaming::withSuffix("Brick_1", 1) == "Brick_1_1");
    CHECK(AssetNaming::withSuffix("Old.Brick", 2) == "Old.Brick_2");
}

TEST_CASE("the claim takes the first candidate the create does not find taken", "[editor][assets][naming]") {
    std::set<std::string> taken = {"Brick", "Brick_1", "Brick_3"};
    std::vector<std::string> tried;
    const auto candidate = [](const std::uint32_t suffix) {
        return AssetNaming::withSuffix("Brick", suffix);
    };
    const auto create = [&taken, &tried](const std::string& path) -> std::error_code {
        tried.push_back(path);
        if (taken.contains(path)) {
            return FileError::AlreadyExists;
        }
        taken.insert(path);
        return {};
    };
    std::string claimed;

    REQUIRE(AssetNaming::claimFirstFree(candidate, create, claimed) == std::error_code{});

    CHECK(claimed == "Brick_2");
    CHECK(tried == std::vector<std::string>{"Brick", "Brick_1", "Brick_2"});

    SECTION("the next claim moves past the name it just took") {
        std::string next;
        REQUIRE(AssetNaming::claimFirstFree(candidate, create, next) == std::error_code{});
        CHECK(next == "Brick_4");
    }
}

TEST_CASE("the claim stops at the first error that is not AlreadyExists and leaves out as it was", "[editor][assets][naming]") {
    int calls = 0;
    const auto candidate = [](const std::uint32_t suffix) {
        return AssetNaming::withSuffix("Brick", suffix);
    };
    const auto create = [&calls](const std::string&) -> std::error_code {
        calls++;
        if (calls == 1) {
            return FileError::AlreadyExists;
        }
        return FileError::IoError;
    };
    std::string claimed = "untouched";

    CHECK(AssetNaming::claimFirstFree(candidate, create, claimed) == FileError::IoError);

    CHECK(claimed == "untouched");
    CHECK(calls == 2);
}
