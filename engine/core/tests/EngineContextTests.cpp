#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <vector>

using TechEngine::EngineContext;
using TechEngine::FileAccess;
using TechEngine::FileResult;
using TechEngine::MountTable;
using TechEngineTests::ScratchDirectory;

// F30's regression test: core holds the context, platform holds the implementation, and no
// editor code is in the link. v1 could not do this at all — its only IFileSystem impl lived
// in the editor, so a shipped runtime had no way to load an asset.
TEST_CASE("a file is read through the context by virtual path", "[core][enginecontext]") {
    const ScratchDirectory scratch{"engineContextRead"};
    scratch.writeFile("demo.txt", "read through a virtual path");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    FileAccess files{mounts};
    const EngineContext engine{files};

    std::vector<std::byte> out;
    REQUIRE(engine.files.read("assets://demo.txt", out) == FileResult::Ok);
    CHECK(std::string(reinterpret_cast<const char*>(out.data()), out.size()) == "read through a virtual path");
}

TEST_CASE("the context carries its miss results through unchanged", "[core][enginecontext]") {
    const ScratchDirectory scratch{"engineContextMisses"};

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    FileAccess files{mounts};
    const EngineContext engine{files};

    std::vector<std::byte> out;
    CHECK(engine.files.read("cache://demo.txt", out) == FileResult::NoMount);
    CHECK(engine.files.read("assets://demo.txt", out) == FileResult::NotFound);
    CHECK(engine.files.read("demo.txt", out) == FileResult::InvalidPath);
}

// The context is a non-owning view (ADR-006 §4's F13 rule), so it must observe a mount
// established after it was built — the composition root wires services before mounts.
TEST_CASE("the context observes the table it was built over", "[core][enginecontext]") {
    const ScratchDirectory scratch{"engineContextLateMount"};
    scratch.writeFile("demo.txt", "late");

    MountTable mounts;
    FileAccess files{mounts};
    const EngineContext engine{files};

    std::vector<std::byte> out;
    REQUIRE(engine.files.read("assets://demo.txt", out) == FileResult::NoMount);

    mounts.mount("assets", scratch.root());
    CHECK(engine.files.read("assets://demo.txt", out) == FileResult::Ok);
}
