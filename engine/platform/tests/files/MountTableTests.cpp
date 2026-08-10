#include <TechEngine/platform/files/MountTable.hpp>

#include "ScratchDirectory.hpp"
#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using TechEngine::FileResult;
using TechEngine::MountTable;
using TechEngineTests::ScratchDirectory;

TEST_CASE("mount registers an alias", "[files][mounttable]") {
    MountTable table;
    table.mount("assets", "/some/root");

    CHECK(table.mountCount() == 1);
    CHECK(table.hasAlias("assets"));
    CHECK_FALSE(table.hasAlias("Assets"));
    CHECK_FALSE(table.hasAlias("cache"));
}

TEST_CASE("unmount removes every entry for an alias", "[files][mounttable]") {
    MountTable table;
    table.mount("assets", "/base", 0);
    table.mount("assets", "/overlay", 100);
    table.mount("cache", "/cache", 0);

    REQUIRE(table.unmount("assets"));
    CHECK(table.mountCount() == 1);
    CHECK_FALSE(table.hasAlias("assets"));
    CHECK(table.hasAlias("cache"));
    CHECK_FALSE(table.unmount("assets"));
}

TEST_CASE("entries are ordered by descending priority, ties in mount order", "[files][mounttable]") {
    MountTable table;
    table.mount("a", "/low", 1);
    table.mount("b", "/high", 100);
    table.mount("c", "/tieFirst", 100);
    table.mount("d", "/tieSecond", 100);

    const auto entries = table.entries();
    REQUIRE(entries.size() == 4);
    CHECK(entries[0].physicalRoot == std::filesystem::path{"/high"});
    CHECK(entries[1].physicalRoot == std::filesystem::path{"/tieFirst"});
    CHECK(entries[2].physicalRoot == std::filesystem::path{"/tieSecond"});
    CHECK(entries[3].physicalRoot == std::filesystem::path{"/low"});
}

TEST_CASE("resolveExisting finds a file under a mounted alias", "[files][mounttable]") {
    const ScratchDirectory scratch{"resolveExisting"};
    scratch.writeFile("ui/icon.png", "png");

    MountTable table;
    table.mount("assets", scratch.root());

    std::filesystem::path resolved;
    REQUIRE(table.resolveExisting("assets://ui/icon.png", resolved) == FileResult::Ok);
    CHECK(resolved == scratch.root() / "ui" / "icon.png");
}

TEST_CASE("resolveExisting resolves an alias-only path to the mount root", "[files][mounttable]") {
    const ScratchDirectory scratch{"resolveRoot"};

    MountTable table;
    table.mount("assets", scratch.root());

    std::filesystem::path resolved;
    REQUIRE(table.resolveExisting("assets://", resolved) == FileResult::Ok);
    CHECK(resolved == scratch.root());
}

TEST_CASE("resolveExisting separates an unknown alias from a missing file", "[files][mounttable]") {
    const ScratchDirectory scratch{"resolveMisses"};

    MountTable table;
    table.mount("assets", scratch.root());

    std::filesystem::path resolved;
    CHECK(table.resolveExisting("cache://ui/icon.png", resolved) == FileResult::NoMount);
    CHECK(table.resolveExisting("assets://ui/icon.png", resolved) == FileResult::NotFound);
    CHECK(table.resolveExisting("ui/icon.png", resolved) == FileResult::InvalidPath);
}

TEST_CASE("resolveExisting prefers the highest-priority mount that has the file", "[files][mounttable]") {
    const ScratchDirectory base{"resolvePriorityBase"};
    const ScratchDirectory overlay{"resolvePriorityOverlay"};
    base.writeFile("ui/icon.png", "base");
    overlay.writeFile("ui/icon.png", "overlay");

    MountTable table;
    table.mount("assets", base.root(), 0);
    table.mount("assets", overlay.root(), 100);

    std::filesystem::path resolved;
    REQUIRE(table.resolveExisting("assets://ui/icon.png", resolved) == FileResult::Ok);
    CHECK(resolved == overlay.root() / "ui" / "icon.png");
}

TEST_CASE("resolveExisting falls through to a lower-priority mount", "[files][mounttable]") {
    const ScratchDirectory base{"resolveFallthroughBase"};
    const ScratchDirectory overlay{"resolveFallthroughOverlay"};
    base.writeFile("ui/icon.png", "base");

    MountTable table;
    table.mount("assets", base.root(), 0);
    table.mount("assets", overlay.root(), 100);

    std::filesystem::path resolved;
    REQUIRE(table.resolveExisting("assets://ui/icon.png", resolved) == FileResult::Ok);
    CHECK(resolved == base.root() / "ui" / "icon.png");
}

// The one case whose OS behaviour differs: Windows opens the file, Linux does not. Both legs
// must report NotFound.
TEST_CASE("resolveExisting is case-sensitive", "[files][mounttable]") {
    const ScratchDirectory scratch{"resolveCase"};
    scratch.writeFile("ui/icon.png", "png");

    MountTable table;
    table.mount("assets", scratch.root());

    std::filesystem::path resolved;
    CHECK(table.resolveExisting("assets://UI/icon.png", resolved) == FileResult::NotFound);
    CHECK(table.resolveExisting("assets://ui/Icon.png", resolved) == FileResult::NotFound);
    CHECK(table.resolveExisting("assets://ui/icon.PNG", resolved) == FileResult::NotFound);
}
