#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <system_error>
#include <vector>

using TechEngine::ImportBlock;
using TechEngine::ImportSettings;
using TechEngine::Reader;
using TechEngine::ReadError;
using TechEngine::UpAxis;
using TechEngine::Writer;

static constexpr std::uint64_t TRAILING_MARKER = 0x747261696C000001;

static ImportBlock makeRootBlock() {
    ImportBlock block;
    block.sourcePath = "assets/common/Kart.gltf";
    block.settings.scale = 0.01F;
    block.settings.upAxis = UpAxis::Z;
    return block;
}

static ImportBlock makeMeshBlock() {
    ImportBlock block;
    block.sourcePath = "assets/common/Kart.gltf";
    block.sourceIndex = 7;
    block.sourceName = "Wheel_Front";
    return block;
}

static std::vector<std::byte> writeBlock(const ImportBlock& block) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    block.write(writer);
    return bytes;
}

static std::uint32_t readByteCount(const std::vector<std::byte>& bytes) {
    std::uint32_t count = 0;
    REQUIRE(bytes.size() >= sizeof(count));
    std::memcpy(&count, bytes.data(), sizeof(count));
    return count;
}

static void setByteCount(std::vector<std::byte>& bytes, const std::uint32_t count) {
    REQUIRE(bytes.size() >= sizeof(count));
    std::memcpy(bytes.data(), &count, sizeof(count));
}

TEST_CASE("an import block written and read back keeps its source path, key and settings", "[core][assets][import]") {
    const ImportBlock written = makeRootBlock();
    const std::vector<std::byte> bytes = writeBlock(written);
    Reader reader{bytes};
    ImportBlock loaded;

    REQUIRE(loaded.read(reader) == std::error_code{});

    CHECK(loaded == written);
    CHECK(reader.remaining() == 0);
}

TEST_CASE("an import block with a source index and name round-trips them", "[core][assets][import]") {
    const ImportBlock written = makeMeshBlock();
    const std::vector<std::byte> bytes = writeBlock(written);
    Reader reader{bytes};
    ImportBlock loaded = makeRootBlock();

    REQUIRE(loaded.read(reader) == std::error_code{});

    CHECK(loaded.sourceIndex == 7);
    CHECK(loaded.sourceName == "Wheel_Front");
    CHECK(loaded.settings == ImportSettings{});
}

TEST_CASE("an empty import block round-trips", "[core][assets][import]") {
    const std::vector<std::byte> bytes = writeBlock(ImportBlock{});
    Reader reader{bytes};
    ImportBlock loaded = makeMeshBlock();

    REQUIRE(loaded.read(reader) == std::error_code{});

    CHECK(loaded == ImportBlock{});
}

TEST_CASE("default settings take the same bytes as any other settings", "[core][assets][import]") {
    ImportBlock defaults = makeRootBlock();
    defaults.settings = ImportSettings{};

    CHECK(writeBlock(defaults).size() == writeBlock(makeRootBlock()).size());
}

TEST_CASE("an import block starts with the byte count of what follows it", "[core][assets][import]") {
    const std::vector<std::byte> bytes = writeBlock(makeMeshBlock());

    CHECK(readByteCount(bytes) == bytes.size() - sizeof(std::uint32_t));
}

TEST_CASE("a reader can step over an import block by its byte count alone", "[core][assets][import]") {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    makeRootBlock().write(writer);
    writer.write(TRAILING_MARKER);
    Reader reader{bytes};
    std::uint32_t count = 0;
    reader.read(count);
    std::vector<std::byte> skipped(count);
    reader.readBytes(skipped);
    std::uint64_t marker = 0;

    reader.read(marker);

    REQUIRE(reader.ok());
    CHECK(marker == TRAILING_MARKER);
    CHECK(reader.remaining() == 0);
}

TEST_CASE("reading an import block leaves the reader at the data after it", "[core][assets][import]") {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    makeMeshBlock().write(writer);
    writer.write(TRAILING_MARKER);
    Reader reader{bytes};
    ImportBlock loaded;

    REQUIRE(loaded.read(reader) == std::error_code{});
    std::uint64_t marker = 0;
    reader.read(marker);

    REQUIRE(reader.ok());
    CHECK(marker == TRAILING_MARKER);
}

TEST_CASE("read rejects an import block whose bytes break the format and leaves the block untouched", "[core][assets][import]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("an up axis past Z") {
        ImportBlock block = makeRootBlock();
        block.settings.upAxis = static_cast<UpAxis>(2);
        bytes = writeBlock(block);
        expected = ReadError::Malformed;
    }

    SECTION("a byte count larger than the block's fields") {
        bytes = writeBlock(makeRootBlock());
        setByteCount(bytes, readByteCount(bytes) + 1);
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the block") {
        bytes = writeBlock(makeRootBlock());
        bytes.resize(bytes.size() - 1);
        expected = ReadError::Truncated;
    }

    SECTION("bytes cut inside the byte count") {
        bytes = writeBlock(makeRootBlock());
        bytes.resize(sizeof(std::uint32_t) - 1);
        expected = ReadError::Truncated;
    }

    Reader reader{bytes};
    ImportBlock loaded = makeMeshBlock();
    REQUIRE(loaded.read(reader) == expected);
    CHECK(loaded == makeMeshBlock());
}
