#include <TechEngine/platform/files/FileAccess.hpp>

#include "ScratchDirectory.hpp"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

using TechEngine::FileAccess;
using TechEngine::FileResult;
using TechEngine::FileStatus;
using TechEngine::MountTable;
using TechEngineTests::ScratchDirectory;

static std::string asString(const std::vector<std::byte>& bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

TEST_CASE("read returns the file's bytes", "[files][fileaccess]") {
    const ScratchDirectory scratch{"readBytes"};
    scratch.writeFile("ui/icon.png", "the bytes");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::byte> out;
    REQUIRE(files.read("assets://ui/icon.png", out) == FileResult::Ok);
    CHECK(asString(out) == "the bytes");
}

TEST_CASE("read replaces the output buffer rather than appending", "[files][fileaccess]") {
    const ScratchDirectory scratch{"readReplaces"};
    scratch.writeFile("a.txt", "short");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::byte> out(64, std::byte{0xAB});
    REQUIRE(files.read("assets://a.txt", out) == FileResult::Ok);
    CHECK(out.size() == 5);
    CHECK(asString(out) == "short");
}

TEST_CASE("read accepts an empty file", "[files][fileaccess]") {
    const ScratchDirectory scratch{"readEmpty"};
    scratch.writeFile("empty.txt", "");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::byte> out(8, std::byte{0xAB});
    REQUIRE(files.read("assets://empty.txt", out) == FileResult::Ok);
    CHECK(out.empty());
}

// ifstream opens a directory successfully on Linux and fails on Windows, so this has to be
// an explicit check rather than a side effect of the open.
TEST_CASE("read of a directory reports IsADirectory", "[files][fileaccess]") {
    const ScratchDirectory scratch{"readDirectory"};
    scratch.writeFile("ui/icon.png", "png");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::byte> out;
    CHECK(files.read("assets://ui", out) == FileResult::IsADirectory);
}

TEST_CASE("read separates its miss kinds and never falls back to a log", "[files][fileaccess]") {
    const ScratchDirectory scratch{"readMisses"};

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::byte> out;
    CHECK(files.read("cache://a.txt", out) == FileResult::NoMount);
    CHECK(files.read("assets://a.txt", out) == FileResult::NotFound);
    CHECK(files.read("a.txt", out) == FileResult::InvalidPath);
}

TEST_CASE("read goes through the mount priority order", "[files][fileaccess]") {
    const ScratchDirectory base{"readPriorityBase"};
    const ScratchDirectory overlay{"readPriorityOverlay"};
    base.writeFile("a.txt", "from base");
    overlay.writeFile("a.txt", "from overlay");

    MountTable mounts;
    mounts.mount("assets", base.root(), 0);
    mounts.mount("assets", overlay.root(), 100);
    const FileAccess files{mounts};

    std::vector<std::byte> out;
    REQUIRE(files.read("assets://a.txt", out) == FileResult::Ok);
    CHECK(asString(out) == "from overlay");
}

TEST_CASE("status describes a file", "[files][fileaccess]") {
    const ScratchDirectory scratch{"statusFile"};
    scratch.writeFile("a.txt", "12345");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    FileStatus out;
    REQUIRE(files.status("assets://a.txt", out) == FileResult::Ok);
    CHECK(out.physicalPath == scratch.root() / "a.txt");
    CHECK_FALSE(out.isDirectory);
    CHECK(out.size == 5);
}

TEST_CASE("status describes a directory", "[files][fileaccess]") {
    const ScratchDirectory scratch{"statusDirectory"};
    scratch.writeFile("ui/icon.png", "png");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    FileStatus out;
    REQUIRE(files.status("assets://ui", out) == FileResult::Ok);
    CHECK(out.isDirectory);
}

// A raw file_time_type tick count lands in the 1e17–1e18 range on both platforms, so the
// upper bound is what proves the epoch was actually converted.
TEST_CASE("status reports lastModified as Unix seconds", "[files][fileaccess]") {
    const ScratchDirectory scratch{"statusTimestamp"};
    scratch.writeFile("a.txt", "now");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    FileStatus out;
    REQUIRE(files.status("assets://a.txt", out) == FileResult::Ok);

    const auto now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    CHECK(out.lastModified > 1700000000);
    CHECK(out.lastModified <= now + 60);
}

TEST_CASE("status separates its miss kinds", "[files][fileaccess]") {
    const ScratchDirectory scratch{"statusMisses"};

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    FileStatus out;
    CHECK(files.status("cache://a.txt", out) == FileResult::NoMount);
    CHECK(files.status("assets://a.txt", out) == FileResult::NotFound);
    CHECK(files.status("a.txt", out) == FileResult::InvalidPath);
}

TEST_CASE("list returns virtual paths, not physical ones", "[files][fileaccess]") {
    const ScratchDirectory scratch{"listVirtual"};
    scratch.writeFile("ui/icon.png", "png");
    scratch.writeFile("ui/nested/deep.png", "png");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::string> out;
    REQUIRE(files.list("assets://ui", false, out) == FileResult::Ok);
    std::ranges::sort(out);
    CHECK(out == std::vector<std::string>{"assets://ui/icon.png", "assets://ui/nested"});
}

TEST_CASE("list recurses when asked", "[files][fileaccess]") {
    const ScratchDirectory scratch{"listRecursive"};
    scratch.writeFile("ui/icon.png", "png");
    scratch.writeFile("ui/nested/deep.png", "png");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::string> out;
    REQUIRE(files.list("assets://ui", true, out) == FileResult::Ok);
    std::ranges::sort(out);
    CHECK(out == std::vector<std::string>{"assets://ui/icon.png", "assets://ui/nested", "assets://ui/nested/deep.png"});
}

TEST_CASE("list of the alias root lists the mount root", "[files][fileaccess]") {
    const ScratchDirectory scratch{"listRoot"};
    scratch.writeFile("ui/icon.png", "png");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::string> out;
    REQUIRE(files.list("assets://", false, out) == FileResult::Ok);
    CHECK(out == std::vector<std::string>{"assets://ui"});
}

TEST_CASE("list of a file reports NotADirectory", "[files][fileaccess]") {
    const ScratchDirectory scratch{"listFile"};
    scratch.writeFile("a.txt", "text");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::vector<std::string> out;
    CHECK(files.list("assets://a.txt", false, out) == FileResult::NotADirectory);
}

// Decided: list does NOT union overlays — it lists the mount that wins the existence walk.
// So a file only the base mount has is readable but not listed.
TEST_CASE("list covers only the winning mount", "[files][fileaccess]") {
    const ScratchDirectory base{"listPriorityBase"};
    const ScratchDirectory overlay{"listPriorityOverlay"};
    base.writeFile("ui/only-in-base.png", "png");
    overlay.writeFile("ui/only-in-overlay.png", "png");

    MountTable mounts;
    mounts.mount("assets", base.root(), 0);
    mounts.mount("assets", overlay.root(), 100);
    const FileAccess files{mounts};

    std::vector<std::string> out;
    REQUIRE(files.list("assets://ui", false, out) == FileResult::Ok);
    CHECK(out == std::vector<std::string>{"assets://ui/only-in-overlay.png"});

    std::vector<std::byte> bytes;
    CHECK(files.read("assets://ui/only-in-base.png", bytes) == FileResult::Ok);
}

TEST_CASE("resolve forwards to the mount table", "[files][fileaccess]") {
    const ScratchDirectory scratch{"resolveForward"};
    scratch.writeFile("a.txt", "text");

    MountTable mounts;
    mounts.mount("assets", scratch.root());
    const FileAccess files{mounts};

    std::filesystem::path out;
    REQUIRE(files.resolve("assets://a.txt", out) == FileResult::Ok);
    CHECK(out == scratch.root() / "a.txt");
    CHECK(files.resolve("assets://missing.txt", out) == FileResult::NotFound);
}
