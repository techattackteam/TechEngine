#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <project/Project.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using TechEngine::FileAccess;
using TechEngine::MountTable;
using TechEngine::Project;
using TechEngine::ProjectResult;
using TechEngineTests::ScratchDirectory;

namespace {
    // mounts is declared before files, so the reference files captures is already alive; the
    // mount() call lands afterwards because FileAccess only stores a pointer to the table.
    struct MountedProject {
        ScratchDirectory scratch;
        MountTable mounts;
        FileAccess files;

        explicit MountedProject(std::string_view name) : scratch{name}, files{mounts} {
            mounts.mount("project", scratch.root());
        }
    };

    constexpr std::string_view GOOD_MANIFEST = "name = \"Sandbox\"\n";
}

TEST_CASE("load reads the name and derives the root", "[editor][project]") {
    MountedProject env{"projectLoadGood"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project project;

    REQUIRE(Project::load(env.files, "project://project.toml", project) == ProjectResult::Ok);
    CHECK(project.name() == "Sandbox");
    CHECK(project.root() == env.scratch.root());
}

TEST_CASE("load returns ParseFailed on a malformed manifest", "[editor][project]") {
    MountedProject env{"projectLoadMalformed"};
    env.scratch.writeFile("project.toml", "name = [unclosed\n");

    Project project;

    CHECK(Project::load(env.files, "project://project.toml", project) == ProjectResult::ParseFailed);
}

TEST_CASE("load returns ReadFailed when the manifest is missing", "[editor][project]") {
    MountedProject env{"projectLoadMissing"};

    Project project;

    CHECK(Project::load(env.files, "project://project.toml", project) == ProjectResult::ReadFailed);
}

TEST_CASE("load returns ReadFailed for a manifest path that escapes the root", "[editor][project]") {
    MountedProject env{"projectLoadEscapes"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project project;

    CHECK(Project::load(env.files, "project://../project.toml", project) == ProjectResult::ReadFailed);
}

TEST_CASE("load returns SchemaInvalid when name is missing", "[editor][project]") {
    MountedProject env{"projectLoadNoName"};
    env.scratch.writeFile("project.toml", "unrelated = 1\n");

    Project project;

    CHECK(Project::load(env.files, "project://project.toml", project) == ProjectResult::SchemaInvalid);
}

TEST_CASE("load returns SchemaInvalid when name is not a string", "[editor][project]") {
    MountedProject env{"projectLoadBadName"};
    env.scratch.writeFile("project.toml", "name = 7\n");

    Project project;

    CHECK(Project::load(env.files, "project://project.toml", project) == ProjectResult::SchemaInvalid);
}

TEST_CASE("save round-trips through load", "[editor][project]") {
    MountedProject env{"projectSaveRoundTrip"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project source;
    REQUIRE(Project::load(env.files, "project://project.toml", source) == ProjectResult::Ok);
    REQUIRE(source.save(env.files, "project://copy.toml") == ProjectResult::Ok);

    Project reloaded;
    REQUIRE(Project::load(env.files, "project://copy.toml", reloaded) == ProjectResult::Ok);
    CHECK(reloaded.name() == source.name());
}
