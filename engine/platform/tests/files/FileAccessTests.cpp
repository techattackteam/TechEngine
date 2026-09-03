#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using TechEngine::FileAccess;
using TechEngine::FileResult;
using TechEngine::FileStatus;
using TechEngine::MountTable;
using TechEngineTests::ScratchDirectory;

namespace {
    // mounts is declared before files, so the reference files captures is already alive; the
    // mount() call lands afterwards because FileAccess only stores a pointer to the table.
    struct MountedScratch {
        ScratchDirectory scratch;
        MountTable mounts;
        FileAccess files;

        explicit MountedScratch(std::string_view name) : scratch{name}, files{mounts} {
            mounts.mount("assets", scratch.root());
        }
    };
}

static std::string asString(const std::vector<std::byte>& bytes) {
    if (bytes.empty()) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

static std::string everyByteValue() {
    std::string payload;
    payload.reserve(256);
    for (int i = 0; i < 256; i++) {
        payload.push_back(static_cast<char>(i));
    }
    return payload;
}

TEST_CASE("read returns the file's bytes", "[files][fileaccess]") {
    MountedScratch env{"readBytes"};
    env.scratch.writeFile("ui/icon.png", "the bytes");

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://ui/icon.png", out) == FileResult::Ok);
    CHECK(asString(out) == "the bytes");
}

// All 256 values, so an embedded NUL cannot truncate and a stream that forgot std::ios::binary
// fails on Windows the moment it rewrites 0x0D 0x0A.
TEST_CASE("read is binary-safe", "[files][fileaccess]") {
    MountedScratch env{"readBinary"};
    const std::string payload = everyByteValue();
    env.scratch.writeFile("blob.bin", payload);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://blob.bin", out) == FileResult::Ok);
    REQUIRE(out.size() == payload.size());
    CHECK(asString(out) == payload);
}

TEST_CASE("read returns a large file whole", "[files][fileaccess]") {
    MountedScratch env{"readLarge"};
    std::string payload;
    payload.reserve(256 * 1024);
    while (payload.size() < 256 * 1024) {
        payload += everyByteValue();
    }
    env.scratch.writeFile("blob.bin", payload);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://blob.bin", out) == FileResult::Ok);
    REQUIRE(out.size() == payload.size());
    CHECK(asString(out) == payload);
}

TEST_CASE("read replaces the output buffer rather than appending", "[files][fileaccess]") {
    MountedScratch env{"readReplaces"};
    env.scratch.writeFile("a.txt", "short");

    std::vector<std::byte> out(64, std::byte{0xAB});
    REQUIRE(env.files.read("assets://a.txt", out) == FileResult::Ok);
    CHECK(out.size() == 5);
    CHECK(asString(out) == "short");
}

TEST_CASE("read accepts an empty file", "[files][fileaccess]") {
    MountedScratch env{"readEmpty"};
    env.scratch.writeFile("empty.txt", "");

    std::vector<std::byte> out(8, std::byte{0xAB});
    REQUIRE(env.files.read("assets://empty.txt", out) == FileResult::Ok);
    CHECK(out.empty());
}

// ifstream opens a directory successfully on Linux and fails on Windows, so this has to be
// an explicit check rather than a side effect of the open.
TEST_CASE("read of a directory reports IsADirectory", "[files][fileaccess]") {
    MountedScratch env{"readDirectory"};
    env.scratch.writeFile("ui/icon.png", "png");

    std::vector<std::byte> out;
    CHECK(env.files.read("assets://ui", out) == FileResult::IsADirectory);
}

TEST_CASE("read separates its miss kinds and never falls back to a log", "[files][fileaccess]") {
    MountedScratch env{"readMisses"};

    std::vector<std::byte> out;
    CHECK(env.files.read("cache://a.txt", out) == FileResult::NoMount);
    CHECK(env.files.read("assets://a.txt", out) == FileResult::NotFound);
    CHECK(env.files.read("a.txt", out) == FileResult::InvalidPath);
}

TEST_CASE("read rejects a path that would escape the mount root", "[files][fileaccess]") {
    MountedScratch env{"readEscape"};
    env.scratch.writeFile("a.txt", "text");

    std::vector<std::byte> out;
    CHECK(env.files.read("assets://../a.txt", out) == FileResult::InvalidPath);
    CHECK(env.files.read("assets://ui/../a.txt", out) == FileResult::InvalidPath);
    CHECK(env.files.read("assets:///etc/passwd", out) == FileResult::InvalidPath);
}

TEST_CASE("status describes a file", "[files][fileaccess]") {
    MountedScratch env{"statusFile"};
    env.scratch.writeFile("a.txt", "12345");

    FileStatus out;
    REQUIRE(env.files.status("assets://a.txt", out) == FileResult::Ok);
    CHECK(out.physicalPath == env.scratch.root() / "a.txt");
    CHECK_FALSE(out.isDirectory);
    CHECK(out.size == 5);
}

TEST_CASE("status describes a directory", "[files][fileaccess]") {
    MountedScratch env{"statusDirectory"};
    env.scratch.writeFile("ui/icon.png", "png");

    FileStatus out;
    REQUIRE(env.files.status("assets://ui", out) == FileResult::Ok);
    CHECK(out.physicalPath == env.scratch.root() / "ui");
    CHECK(out.isDirectory);
}

TEST_CASE("status describes the alias root", "[files][fileaccess]") {
    MountedScratch env{"statusRoot"};
    env.scratch.writeFile("ui/icon.png", "png");

    FileStatus out;
    REQUIRE(env.files.status("assets://", out) == FileResult::Ok);
    CHECK(out.physicalPath == env.scratch.root());
    CHECK(out.isDirectory);
}

// A raw file_time_type tick count lands in the 1e17–1e18 range on both platforms, so the
// upper bound is what proves the epoch was actually converted.
TEST_CASE("status reports lastModified as Unix seconds", "[files][fileaccess]") {
    MountedScratch env{"statusTimestamp"};
    env.scratch.writeFile("a.txt", "now");

    FileStatus out;
    REQUIRE(env.files.status("assets://a.txt", out) == FileResult::Ok);

    const auto now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    CHECK(out.lastModified > 1700000000);
    CHECK(out.lastModified <= now + 60);
}

TEST_CASE("status separates its miss kinds", "[files][fileaccess]") {
    MountedScratch env{"statusMisses"};

    FileStatus out;
    CHECK(env.files.status("cache://a.txt", out) == FileResult::NoMount);
    CHECK(env.files.status("assets://a.txt", out) == FileResult::NotFound);
    CHECK(env.files.status("a.txt", out) == FileResult::InvalidPath);
}

TEST_CASE("a failed status leaves the output untouched", "[files][fileaccess]") {
    MountedScratch env{"statusUntouched"};

    FileStatus out;
    out.physicalPath = "sentinel";
    out.isDirectory = true;
    out.size = 999;
    out.lastModified = 777;

    REQUIRE(env.files.status("assets://missing.txt", out) == FileResult::NotFound);
    CHECK(out.physicalPath == std::filesystem::path{"sentinel"});
    CHECK(out.isDirectory);
    CHECK(out.size == 999);
    CHECK(out.lastModified == 777);
}

TEST_CASE("list returns virtual paths, not physical ones", "[files][fileaccess]") {
    MountedScratch env{"listVirtual"};
    env.scratch.writeFile("ui/icon.png", "png");
    env.scratch.writeFile("ui/nested/deep.png", "png");

    std::vector<std::string> out;
    REQUIRE(env.files.list("assets://ui", false, out) == FileResult::Ok);
    std::ranges::sort(out);
    CHECK(out == std::vector<std::string>{"assets://ui/icon.png", "assets://ui/nested"});
}

TEST_CASE("list recurses when asked", "[files][fileaccess]") {
    MountedScratch env{"listRecursive"};
    env.scratch.writeFile("ui/icon.png", "png");
    env.scratch.writeFile("ui/nested/deep.png", "png");

    std::vector<std::string> out;
    REQUIRE(env.files.list("assets://ui", true, out) == FileResult::Ok);
    std::ranges::sort(out);
    CHECK(out == std::vector<std::string>{"assets://ui/icon.png", "assets://ui/nested", "assets://ui/nested/deep.png"});
}

TEST_CASE("list of the alias root lists the mount root", "[files][fileaccess]") {
    MountedScratch env{"listRoot"};
    env.scratch.writeFile("ui/icon.png", "png");

    std::vector<std::string> out;
    REQUIRE(env.files.list("assets://", false, out) == FileResult::Ok);
    CHECK(out == std::vector<std::string>{"assets://ui"});
}

TEST_CASE("list of an empty directory succeeds with no entries", "[files][fileaccess]") {
    MountedScratch env{"listEmpty"};
    env.scratch.makeDirectory("ui");

    std::vector<std::string> out;
    REQUIRE(env.files.list("assets://ui", false, out) == FileResult::Ok);
    CHECK(out.empty());
}

TEST_CASE("list replaces the output vector rather than appending", "[files][fileaccess]") {
    MountedScratch env{"listReplaces"};
    env.scratch.writeFile("ui/icon.png", "png");

    std::vector<std::string> out{"stale://entry", "another://entry"};
    REQUIRE(env.files.list("assets://ui", false, out) == FileResult::Ok);
    CHECK(out == std::vector<std::string>{"assets://ui/icon.png"});
}

TEST_CASE("list of a file reports NotADirectory", "[files][fileaccess]") {
    MountedScratch env{"listFile"};
    env.scratch.writeFile("a.txt", "text");

    std::vector<std::string> out;
    CHECK(env.files.list("assets://a.txt", false, out) == FileResult::NotADirectory);
}

TEST_CASE("list separates its miss kinds", "[files][fileaccess]") {
    MountedScratch env{"listMisses"};

    std::vector<std::string> out;
    CHECK(env.files.list("cache://ui", false, out) == FileResult::NoMount);
    CHECK(env.files.list("assets://ui", false, out) == FileResult::NotFound);
    CHECK(env.files.list("ui", false, out) == FileResult::InvalidPath);
}

TEST_CASE("resolve forwards to the mount table", "[files][fileaccess]") {
    MountedScratch env{"resolveForward"};
    env.scratch.writeFile("a.txt", "text");

    std::filesystem::path out;
    REQUIRE(env.files.resolve("assets://a.txt", out) == FileResult::Ok);
    CHECK(out == env.scratch.root() / "a.txt");
    CHECK(env.files.resolve("assets://missing.txt", out) == FileResult::NotFound);
    CHECK(env.files.resolve("cache://a.txt", out) == FileResult::NoMount);
    CHECK(env.files.resolve("a.txt", out) == FileResult::InvalidPath);
}

// MountTable pins the rule; these pin that the whole surface goes through it rather than
// probing the filesystem on its own.
TEST_CASE("every entry point is case-sensitive", "[files][fileaccess]") {
    MountedScratch env{"caseSurface"};
    env.scratch.writeFile("ui/icon.png", "png");

    std::vector<std::byte> bytes;
    CHECK(env.files.read("assets://ui/Icon.png", bytes) == FileResult::NotFound);

    FileStatus status;
    CHECK(env.files.status("assets://UI/icon.png", status) == FileResult::NotFound);

    std::vector<std::string> entries;
    CHECK(env.files.list("assets://UI", false, entries) == FileResult::NotFound);

    std::filesystem::path resolved;
    CHECK(env.files.resolve("assets://ui/ICON.PNG", resolved) == FileResult::NotFound);
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

TEST_CASE("read falls through to a lower-priority mount", "[files][fileaccess]") {
    const ScratchDirectory base{"readFallthroughBase"};
    const ScratchDirectory overlay{"readFallthroughOverlay"};
    base.writeFile("a.txt", "from base");
    overlay.writeFile("b.txt", "from overlay");

    MountTable mounts;
    mounts.mount("assets", base.root(), 0);
    mounts.mount("assets", overlay.root(), 100);
    const FileAccess files{mounts};

    std::vector<std::byte> out;
    REQUIRE(files.read("assets://a.txt", out) == FileResult::Ok);
    CHECK(asString(out) == "from base");
}

// list does not union overlays — it lists the mount that wins the existence walk, so a file
// only the base mount has is readable but never listed.
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

static std::vector<std::byte> asBytes(std::string_view text) {
    std::vector<std::byte> bytes(text.size());
    for (std::size_t i = 0; i < text.size(); i++) {
        bytes[i] = static_cast<std::byte>(text[i]);
    }
    return bytes;
}

TEST_CASE("write then read round-trips the bytes", "[files][fileaccess]") {
    MountedScratch env{"writeRoundTrip"};
    const std::vector<std::byte> payload = asBytes("baked level bytes");

    REQUIRE(env.files.write("assets://level.bin", payload) == FileResult::Ok);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://level.bin", out) == FileResult::Ok);
    CHECK(out == payload);
}

// The same all-256-values guard the read side carries: a stream that forgot std::ios::binary
// rewrites 0x0D 0x0A on Windows, and an embedded NUL must not truncate.
TEST_CASE("write preserves every byte value", "[files][fileaccess]") {
    MountedScratch env{"writeEveryByte"};
    const std::vector<std::byte> payload = asBytes(everyByteValue());

    REQUIRE(env.files.write("assets://raw.bin", payload) == FileResult::Ok);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://raw.bin", out) == FileResult::Ok);
    CHECK(out == payload);
}

TEST_CASE("write truncates an existing file rather than appending", "[files][fileaccess]") {
    MountedScratch env{"writeTruncates"};
    env.scratch.writeFile("level.bin", "a much longer previous payload");

    const std::vector<std::byte> payload = asBytes("short");
    REQUIRE(env.files.write("assets://level.bin", payload) == FileResult::Ok);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://level.bin", out) == FileResult::Ok);
    CHECK(asString(out) == "short");
}

TEST_CASE("write accepts an empty span and leaves an empty file", "[files][fileaccess]") {
    MountedScratch env{"writeEmpty"};

    REQUIRE(env.files.write("assets://empty.bin", {}) == FileResult::Ok);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://empty.bin", out) == FileResult::Ok);
    CHECK(out.empty());
}

TEST_CASE("write lands in the highest-priority mount", "[files][fileaccess]") {
    ScratchDirectory base{"writePriorityBase"};
    ScratchDirectory overlay{"writePriorityOverlay"};

    MountTable mounts;
    FileAccess files{mounts};
    mounts.mount("assets", base.root(), 0);
    mounts.mount("assets", overlay.root(), 100);

    REQUIRE(files.write("assets://level.bin", asBytes("overlay")) == FileResult::Ok);
    CHECK(std::filesystem::exists(overlay.root() / "level.bin"));
    CHECK_FALSE(std::filesystem::exists(base.root() / "level.bin"));
}

TEST_CASE("write refuses an unmounted alias, the mount root, and a directory", "[files][fileaccess]") {
    MountedScratch env{"writeRefusals"};
    env.scratch.makeDirectory("meshes");

    CHECK(env.files.write("cache://level.bin", asBytes("x")) == FileResult::NoMount);
    CHECK(env.files.write("assets://", asBytes("x")) == FileResult::InvalidPath);
    CHECK(env.files.write("assets://meshes", asBytes("x")) == FileResult::IsADirectory);
}

TEST_CASE("write does not create a missing parent directory", "[files][fileaccess]") {
    MountedScratch env{"writeMissingParent"};

    CHECK(env.files.write("assets://absent/level.bin", asBytes("x")) == FileResult::NotFound);
}

TEST_CASE("createDirectory creates the directory and its missing parents", "[files][fileaccess]") {
    MountedScratch env{"createDirectoryParents"};

    REQUIRE(env.files.createDirectory("assets://meshes/props/crates") == FileResult::Ok);
    CHECK(std::filesystem::is_directory(env.scratch.root() / "meshes/props/crates"));
}

TEST_CASE("createDirectory refuses a destination that already exists", "[files][fileaccess]") {
    MountedScratch env{"createDirectoryExists"};
    env.scratch.makeDirectory("meshes");
    env.scratch.writeFile("ui/icon.png", "x");

    CHECK(env.files.createDirectory("assets://meshes") == FileResult::AlreadyExists);
    CHECK(env.files.createDirectory("assets://ui/icon.png") == FileResult::AlreadyExists);
}

TEST_CASE("createDirectory refuses an unmounted alias and the mount root", "[files][fileaccess]") {
    MountedScratch env{"createDirectoryRefusals"};

    CHECK(env.files.createDirectory("cache://meshes") == FileResult::NoMount);
    CHECK(env.files.createDirectory("assets://") == FileResult::InvalidPath);
}

TEST_CASE("remove deletes a file and an empty directory", "[files][fileaccess]") {
    MountedScratch env{"removeBasics"};
    env.scratch.writeFile("ui/icon.png", "x");
    env.scratch.makeDirectory("empty");

    REQUIRE(env.files.remove("assets://ui/icon.png", false) == FileResult::Ok);
    CHECK_FALSE(std::filesystem::exists(env.scratch.root() / "ui/icon.png"));

    REQUIRE(env.files.remove("assets://empty", false) == FileResult::Ok);
    CHECK_FALSE(std::filesystem::exists(env.scratch.root() / "empty"));
}

TEST_CASE("remove reports a path that is not there", "[files][fileaccess]") {
    MountedScratch env{"removeMissing"};

    CHECK(env.files.remove("assets://absent.bin", false) == FileResult::NotFound);
}

// The recursive flag is the whole difference: without it a populated directory is refused
// rather than emptied, so a caller cannot delete a tree by accident.
TEST_CASE("remove refuses a non-empty directory unless recursive", "[files][fileaccess]") {
    MountedScratch env{"removeNonEmpty"};
    env.scratch.writeFile("meshes/crate.bin", "x");

    CHECK(env.files.remove("assets://meshes", false) == FileResult::NotEmpty);
    CHECK(std::filesystem::exists(env.scratch.root() / "meshes/crate.bin"));

    REQUIRE(env.files.remove("assets://meshes", true) == FileResult::Ok);
    CHECK_FALSE(std::filesystem::exists(env.scratch.root() / "meshes"));
}

TEST_CASE("copy duplicates a file and leaves the source in place", "[files][fileaccess]") {
    MountedScratch env{"copyFile"};
    env.scratch.writeFile("ui/icon.png", "the bytes");

    REQUIRE(env.files.copy("assets://ui/icon.png", "assets://ui/icon.backup.png") == FileResult::Ok);

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://ui/icon.backup.png", out) == FileResult::Ok);
    CHECK(asString(out) == "the bytes");
    CHECK(std::filesystem::exists(env.scratch.root() / "ui/icon.png"));
}

TEST_CASE("copy carries a directory's contents", "[files][fileaccess]") {
    MountedScratch env{"copyDirectory"};
    env.scratch.writeFile("meshes/props/crate.bin", "x");

    REQUIRE(env.files.copy("assets://meshes", "assets://meshes.backup") == FileResult::Ok);
    CHECK(std::filesystem::exists(env.scratch.root() / "meshes.backup/props/crate.bin"));
}

TEST_CASE("copy refuses an existing destination and a missing parent", "[files][fileaccess]") {
    MountedScratch env{"copyRefusals"};
    env.scratch.writeFile("ui/icon.png", "x");
    env.scratch.writeFile("ui/taken.png", "y");

    CHECK(env.files.copy("assets://ui/icon.png", "assets://ui/taken.png") == FileResult::AlreadyExists);
    CHECK(env.files.copy("assets://ui/icon.png", "assets://absent/icon.png") == FileResult::NotFound);
    CHECK(env.files.copy("assets://absent.png", "assets://ui/copy.png") == FileResult::NotFound);
}

// The source goes through the read path, so it falls through to a lower-priority mount the
// way read does. resolveForCreate would stop at the overlay and report NotFound for a file
// the caller can read.
TEST_CASE("copy resolves its source through the mount priority order", "[files][fileaccess]") {
    ScratchDirectory base{"copySourceBase"};
    ScratchDirectory overlay{"copySourceOverlay"};
    base.writeFile("ui/icon.png", "from the base mount");

    MountTable mounts;
    FileAccess files{mounts};
    mounts.mount("assets", base.root(), 0);
    mounts.mount("assets", overlay.root(), 100);

    REQUIRE(files.copy("assets://ui/icon.png", "assets://copy.png") == FileResult::Ok);
    CHECK(std::filesystem::exists(overlay.root() / "copy.png"));
}

TEST_CASE("move relocates a file and removes the source", "[files][fileaccess]") {
    MountedScratch env{"moveFile"};
    env.scratch.writeFile("ui/icon.png", "the bytes");
    env.scratch.makeDirectory("sprites");

    REQUIRE(env.files.move("assets://ui/icon.png", "assets://sprites/icon.png") == FileResult::Ok);
    CHECK_FALSE(std::filesystem::exists(env.scratch.root() / "ui/icon.png"));

    std::vector<std::byte> out;
    REQUIRE(env.files.read("assets://sprites/icon.png", out) == FileResult::Ok);
    CHECK(asString(out) == "the bytes");
}

TEST_CASE("move refuses an existing destination and a missing source", "[files][fileaccess]") {
    MountedScratch env{"moveRefusals"};
    env.scratch.writeFile("ui/icon.png", "x");
    env.scratch.writeFile("ui/taken.png", "y");

    CHECK(env.files.move("assets://ui/icon.png", "assets://ui/taken.png") == FileResult::AlreadyExists);
    CHECK(env.files.move("assets://absent.png", "assets://ui/moved.png") == FileResult::NotFound);
    CHECK(std::filesystem::exists(env.scratch.root() / "ui/icon.png"));
}

TEST_CASE("rename changes the name in place", "[files][fileaccess]") {
    MountedScratch env{"renameFile"};
    env.scratch.writeFile("ui/icon.png", "the bytes");

    REQUIRE(env.files.rename("assets://ui/icon.png", "logo.png") == FileResult::Ok);
    CHECK_FALSE(std::filesystem::exists(env.scratch.root() / "ui/icon.png"));
    CHECK(std::filesystem::exists(env.scratch.root() / "ui/logo.png"));
}

// newName is one component, never a path. Accepting a separator would turn rename into a
// move that skips the destination's mount resolution entirely.
TEST_CASE("rename refuses a new name carrying a separator", "[files][fileaccess]") {
    MountedScratch env{"renameSeparator"};
    env.scratch.writeFile("ui/icon.png", "x");

    CHECK(env.files.rename("assets://ui/icon.png", "sprites/logo.png") == FileResult::InvalidPath);
    CHECK(env.files.rename("assets://ui/icon.png", "") == FileResult::InvalidPath);
}

TEST_CASE("rename refuses a name already taken and a missing source", "[files][fileaccess]") {
    MountedScratch env{"renameRefusals"};
    env.scratch.writeFile("ui/icon.png", "x");
    env.scratch.writeFile("ui/logo.png", "y");

    CHECK(env.files.rename("assets://ui/icon.png", "logo.png") == FileResult::AlreadyExists);
    CHECK(env.files.rename("assets://absent.png", "logo.png") == FileResult::NotFound);
}
