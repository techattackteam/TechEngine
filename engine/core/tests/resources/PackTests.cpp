#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/Pack.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::MountTable;
using TechEngine::PACK_ALIGNMENT;
using TechEngine::PACK_ENTRY_SIZE;
using TechEngine::PACK_HEADER_SIZE;
using TechEngine::PackEntry;
using TechEngine::PackReader;
using TechEngine::ReadError;
using TechEngine::ResourceId;
using TechEngine::StringId;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view PACK_PATH = "packs://test.tepack";

static constexpr StringId PACK_SCENE_TYPE{"Tests.PackScene"};
static constexpr StringId PACK_MESH_TYPE{"Tests.PackMesh"};

struct PackSampleEntry {
    ResourceId id;
    StringId typeTag;
    std::string_view name;
    std::string_view payload;
};

static constexpr PackSampleEntry PACK_SCENE{ResourceId(0x3000), PACK_SCENE_TYPE, "assets/common/level.tescene", "s"};
static constexpr PackSampleEntry PACK_MESH{ResourceId(0x1000), PACK_MESH_TYPE, "assets/client/cube.temesh", "sixteen bytes!!!"};
static constexpr PackSampleEntry PACK_TEXTURE{ResourceId(0x2000), PACK_MESH_TYPE, "assets/client/odd-name.bin", "thirty-three bytes of payload...."};

static constexpr std::size_t PACK_COUNT_POSITION = TechEngine::BLOB_HEADER_SIZE;
static constexpr std::size_t PACK_BOOT_OFFSET_POSITION = PACK_COUNT_POSITION + 4;
static constexpr std::size_t PACK_BOOT_SIZE_POSITION = PACK_BOOT_OFFSET_POSITION + 8;

static constexpr std::size_t PACK_ENTRY_ID = 0;
static constexpr std::size_t PACK_ENTRY_OFFSET = 16;
static constexpr std::size_t PACK_ENTRY_SIZE_FIELD = 24;
static constexpr std::size_t PACK_ENTRY_NAME = 32;

class PackScratch {
public:
    explicit PackScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_mounts.mount("packs", m_scratch.root());
    }

    FileAccess& files() {
        return m_files;
    }

    void store(const std::span<const std::byte> bytes) {
        REQUIRE(m_files.write(PACK_PATH, bytes) == std::error_code{});
    }

private:
    ScratchDirectory m_scratch;
    MountTable m_mounts;
    FileAccess m_files;
};

static std::span<const std::byte> viewOf(const std::string_view text) {
    return std::as_bytes(std::span<const char>{text.data(), text.size()});
}

static std::vector<std::byte> bytesOf(const std::string_view text) {
    const std::span<const std::byte> view = viewOf(text);
    return {view.begin(), view.end()};
}

static std::vector<std::byte> writeSamplePack(const std::span<const std::byte> bootRecord = {}) {
    TechEngine::PackWriter writer;
    writer.add({PACK_SCENE.id, PACK_SCENE.typeTag, PACK_SCENE.name, viewOf(PACK_SCENE.payload)});
    writer.add({PACK_MESH.id, PACK_MESH.typeTag, PACK_MESH.name, viewOf(PACK_MESH.payload)});
    writer.add({PACK_TEXTURE.id, PACK_TEXTURE.typeTag, PACK_TEXTURE.name, viewOf(PACK_TEXTURE.payload)});
    if (!bootRecord.empty()) {
        writer.setBootRecord(bootRecord);
    }
    std::vector<std::byte> bytes;
    writer.write(bytes);
    return bytes;
}

static std::size_t entryField(const std::size_t row, const std::size_t field) {
    return static_cast<std::size_t>(PACK_HEADER_SIZE + row * PACK_ENTRY_SIZE) + field;
}

template<typename T>
static T peek(const std::vector<std::byte>& bytes, const std::size_t position) {
    REQUIRE(position + sizeof(T) <= bytes.size());
    T value = 0;
    std::memcpy(&value, bytes.data() + position, sizeof(T));
    return value;
}

template<typename T>
static void patch(std::vector<std::byte>& bytes, const std::size_t position, const T value) {
    REQUIRE(position + sizeof(T) <= bytes.size());
    std::memcpy(bytes.data() + position, &value, sizeof(T));
}

TEST_CASE("a pack's header carries its kind, no flags, the entry count and no boot record", "[core][resources]") {
    const std::vector<std::byte> bytes = writeSamplePack();
    TechEngine::Reader reader{bytes};
    TechEngine::BlobHeader header;
    std::uint32_t count = 0;
    std::uint64_t bootOffset = 1;
    std::uint64_t bootSize = 1;

    reader.readHeader(TechEngine::PACK_KIND, header);
    reader.read(count);
    reader.read(bootOffset);
    reader.read(bootSize);

    REQUIRE(reader.ok());
    CHECK(header.flags == 0);
    CHECK(count == 3);
    CHECK(bootOffset == 0);
    CHECK(bootSize == 0);
}

TEST_CASE("a pack's table is sorted by id whatever order the entries were given in", "[core][resources]") {
    const std::vector<std::byte> bytes = writeSamplePack();

    CHECK(peek<std::uint64_t>(bytes, entryField(0, PACK_ENTRY_ID)) == PACK_MESH.id.value());
    CHECK(peek<std::uint64_t>(bytes, entryField(1, PACK_ENTRY_ID)) == PACK_TEXTURE.id.value());
    CHECK(peek<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_ID)) == PACK_SCENE.id.value());
}

TEST_CASE("a pack's names block holds each entry's name at the table's name offset", "[core][resources]") {
    const std::vector<std::byte> bytes = writeSamplePack();
    const std::size_t namesStart = entryField(3, 0);

    std::vector<std::string> names;
    for (std::size_t row = 0; row < 3; row++) {
        const std::uint32_t nameOffset = peek<std::uint32_t>(bytes, entryField(row, PACK_ENTRY_NAME));
        REQUIRE(namesStart + nameOffset < bytes.size());
        TechEngine::Reader reader{std::span<const std::byte>{bytes}.subspan(namesStart + nameOffset)};
        std::string name;
        reader.read(name);
        REQUIRE(reader.ok());
        names.push_back(name);
    }

    CHECK(names == std::vector<std::string>{std::string(PACK_MESH.name), std::string(PACK_TEXTURE.name), std::string(PACK_SCENE.name)});
}

TEST_CASE("every payload in a pack starts at a 16-byte-aligned offset after the table", "[core][resources]") {
    std::vector<std::vector<std::byte>> payloads;
    std::vector<std::string> names;
    for (std::size_t size = 1; size <= 33; size++) {
        payloads.emplace_back(size, static_cast<std::byte>(size));
        names.emplace_back(size % 7 + 1, 'n');
    }
    TechEngine::PackWriter writer;
    for (std::size_t index = 0; index < payloads.size(); index++) {
        writer.add({ResourceId(index + 1), PACK_MESH_TYPE, names[index], payloads[index]});
    }
    std::vector<std::byte> bytes;
    writer.write(bytes);

    for (std::size_t row = 0; row < payloads.size(); row++) {
        INFO("row " << row);
        const std::uint64_t offset = peek<std::uint64_t>(bytes, entryField(row, PACK_ENTRY_OFFSET));
        const std::uint64_t size = peek<std::uint64_t>(bytes, entryField(row, PACK_ENTRY_SIZE_FIELD));

        CHECK(offset % PACK_ALIGNMENT == 0);
        CHECK(offset >= entryField(payloads.size(), 0));
        REQUIRE(size == payloads[row].size());
        REQUIRE(offset + size <= bytes.size());
        CHECK(std::equal(payloads[row].begin(), payloads[row].end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset)));
    }
}

TEST_CASE("several entries round-trip through a pack", "[core][resources]") {
    PackScratch scratch{"packRoundTrip"};
    scratch.store(writeSamplePack());
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == std::error_code{});

    for (const PackSampleEntry& sample: {PACK_SCENE, PACK_MESH, PACK_TEXTURE}) {
        INFO(sample.name);
        PackEntry entry;
        REQUIRE(reader.find(sample.id, entry));
        CHECK(entry.id == sample.id);
        CHECK(entry.typeTag == sample.typeTag);
        CHECK(entry.size == sample.payload.size());
        CHECK(entry.offset % PACK_ALIGNMENT == 0);

        std::vector<std::byte> out;
        REQUIRE(reader.read(entry, out) == std::error_code{});
        CHECK(out == bytesOf(sample.payload));
    }
}

TEST_CASE("an empty pack opens and holds nothing", "[core][resources]") {
    PackScratch scratch{"packEmpty"};
    const TechEngine::PackWriter writer;
    std::vector<std::byte> bytes;
    writer.write(bytes);
    scratch.store(bytes);
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == std::error_code{});

    PackEntry entry;
    CHECK_FALSE(reader.find(PACK_MESH.id, entry));
    CHECK_FALSE(reader.bootRecord(entry));
}

TEST_CASE("an id the pack does not hold is reported absent", "[core][resources]") {
    PackScratch scratch{"packAbsent"};
    scratch.store(writeSamplePack());
    PackReader reader{scratch.files()};
    PackEntry entry;

    CHECK_FALSE(reader.find(PACK_MESH.id, entry));

    REQUIRE(reader.open(PACK_PATH) == std::error_code{});

    CHECK_FALSE(reader.find(ResourceId(0x0FFF), entry));
    CHECK_FALSE(reader.find(ResourceId(0x2500), entry));
    CHECK_FALSE(reader.find(ResourceId(0x3001), entry));
    CHECK_FALSE(reader.find(ResourceId{}, entry));
}

TEST_CASE("an entry's bytes come from the file when it is read, not when the pack opens", "[core][resources]") {
    PackScratch scratch{"packReadLate"};
    std::vector<std::byte> bytes = writeSamplePack();
    scratch.store(bytes);
    PackReader reader{scratch.files()};
    REQUIRE(reader.open(PACK_PATH) == std::error_code{});
    PackEntry entry;
    REQUIRE(reader.find(PACK_MESH.id, entry));

    const std::vector<std::byte> replaced(PACK_MESH.payload.size(), std::byte{'X'});
    std::copy(replaced.begin(), replaced.end(), bytes.begin() + static_cast<std::ptrdiff_t>(entry.offset));
    scratch.store(bytes);

    std::vector<std::byte> out;
    REQUIRE(reader.read(entry, out) == std::error_code{});
    CHECK(out == replaced);
}

TEST_CASE("a boot record written into a pack is read back by its range", "[core][resources]") {
    PackScratch scratch{"packBootRecord"};
    constexpr std::string_view boot = "boot record sections";
    scratch.store(writeSamplePack(viewOf(boot)));
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == std::error_code{});

    PackEntry bootEntry;
    REQUIRE(reader.bootRecord(bootEntry));
    CHECK(bootEntry.size == boot.size());
    std::vector<std::byte> out;
    REQUIRE(reader.read(bootEntry, out) == std::error_code{});
    CHECK(out == bytesOf(boot));

    PackEntry entry;
    REQUIRE(reader.find(PACK_SCENE.id, entry));
    REQUIRE(reader.read(entry, out) == std::error_code{});
    CHECK(out == bytesOf(PACK_SCENE.payload));
}

TEST_CASE("a pack written without a boot record reports none", "[core][resources]") {
    PackScratch scratch{"packNoBootRecord"};
    scratch.store(writeSamplePack());
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == std::error_code{});

    PackEntry bootEntry;
    CHECK_FALSE(reader.bootRecord(bootEntry));
}

TEST_CASE("every truncation of a pack's header or table fails soft as Truncated", "[core][resources]") {
    PackScratch scratch{"packTruncated"};
    const std::vector<std::byte> bytes = writeSamplePack();
    const std::size_t tableEnd = entryField(3, 0);
    REQUIRE(bytes.size() > tableEnd);

    for (std::size_t length = 0; length < tableEnd; length++) {
        INFO("truncated to " << length << " of " << bytes.size() << " bytes");
        scratch.store(std::span<const std::byte>{bytes}.first(length));
        PackReader reader{scratch.files()};

        REQUIRE(reader.open(PACK_PATH) == ReadError::Truncated);

        PackEntry entry;
        REQUIRE_FALSE(reader.find(PACK_MESH.id, entry));
    }
}

TEST_CASE("a pack truncated inside its payloads is Malformed at open", "[core][resources]") {
    PackScratch scratch{"packTruncatedPayload"};
    const std::vector<std::byte> bytes = writeSamplePack();
    std::uint64_t payloadEnd = 0;
    for (std::size_t row = 0; row < 3; row++) {
        payloadEnd = std::max(payloadEnd, peek<std::uint64_t>(bytes, entryField(row, PACK_ENTRY_OFFSET)) + peek<std::uint64_t>(bytes, entryField(row, PACK_ENTRY_SIZE_FIELD)));
    }
    REQUIRE(payloadEnd <= bytes.size());
    scratch.store(std::span<const std::byte>{bytes}.first(static_cast<std::size_t>(payloadEnd) - 1));
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == ReadError::Malformed);

    PackEntry entry;
    REQUIRE_FALSE(reader.find(PACK_MESH.id, entry));
}

TEST_CASE("a pack of another kind or version is rejected", "[core][resources]") {
    PackScratch scratch{"packKind"};
    std::vector<std::byte> bytes = writeSamplePack();
    PackReader reader{scratch.files()};

    SECTION("another kind's magic") {
        patch<std::uint32_t>(bytes, 0, TechEngine::PACK_KIND.magic + 1);
        scratch.store(bytes);

        REQUIRE(reader.open(PACK_PATH) == ReadError::BadMagic);
    }

    SECTION("another version") {
        patch<std::uint16_t>(bytes, 4, static_cast<std::uint16_t>(TechEngine::PACK_KIND.version + 1));
        scratch.store(bytes);

        REQUIRE(reader.open(PACK_PATH) == ReadError::BadVersion);
    }

    PackEntry entry;
    REQUIRE_FALSE(reader.find(PACK_MESH.id, entry));
}

TEST_CASE("a corrupt pack table is Malformed and leaves the reader empty", "[core][resources]") {
    PackScratch scratch{"packCorruptTable"};
    std::vector<std::byte> bytes = writeSamplePack();

    SECTION("ids out of order") {
        patch<std::uint64_t>(bytes, entryField(1, PACK_ENTRY_ID), PACK_SCENE.id.value());
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_ID), PACK_TEXTURE.id.value());
    }

    SECTION("a duplicate id") {
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_ID), PACK_TEXTURE.id.value());
    }

    SECTION("a null id") {
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_ID), 0);
    }

    SECTION("a payload offset that is not 16-byte aligned") {
        const std::uint64_t offset = peek<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_OFFSET));
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_OFFSET), offset + 1);
    }

    SECTION("a payload offset inside the table") {
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_OFFSET), PACK_ALIGNMENT * 2);
    }

    SECTION("a payload that runs past the end of the file") {
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_SIZE_FIELD), bytes.size());
    }

    SECTION("a payload range that overflows") {
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_OFFSET), 0xFFFFFFFFFFFFFFF0);
        patch<std::uint64_t>(bytes, entryField(2, PACK_ENTRY_SIZE_FIELD), 0x20);
    }

    SECTION("a boot record that runs past the end of the file") {
        bytes = writeSamplePack(viewOf("boot record sections"));
        patch<std::uint64_t>(bytes, PACK_BOOT_SIZE_POSITION, bytes.size());
    }

    SECTION("a boot record range that overflows") {
        bytes = writeSamplePack(viewOf("boot record sections"));
        patch<std::uint64_t>(bytes, PACK_BOOT_OFFSET_POSITION, 0xFFFFFFFFFFFFFFF0);
        patch<std::uint64_t>(bytes, PACK_BOOT_SIZE_POSITION, 0x20);
    }

    scratch.store(bytes);
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == ReadError::Malformed);

    PackEntry entry;
    REQUIRE_FALSE(reader.find(PACK_MESH.id, entry));
    REQUIRE_FALSE(reader.bootRecord(entry));
}

TEST_CASE("a missing pack is reported with the file error unchanged", "[core][resources]") {
    PackScratch scratch{"packMissing"};
    PackReader reader{scratch.files()};

    REQUIRE(reader.open(PACK_PATH) == FileError::NotFound);
}

TEST_CASE("an entry read from a pack that has gone fails soft and leaves the out-param untouched", "[core][resources]") {
    PackScratch scratch{"packGone"};
    scratch.store(writeSamplePack());
    PackReader reader{scratch.files()};
    REQUIRE(reader.open(PACK_PATH) == std::error_code{});
    PackEntry entry;
    REQUIRE(reader.find(PACK_MESH.id, entry));
    REQUIRE(scratch.files().remove(PACK_PATH, false) == std::error_code{});

    std::vector<std::byte> out = bytesOf("untouched");
    REQUIRE(reader.read(entry, out) == FileError::NotFound);
    CHECK(out == bytesOf("untouched"));
}

TEST_CASE("adding a null or duplicate id to a pack is a programmer error", "[core][resources]") {
    TechEngine::PackWriter writer;
    const TechEngineTests::FatalAssertGuard guard;

    SECTION("a null id") {
        REQUIRE_THROWS_AS(writer.add({ResourceId{}, PACK_MESH_TYPE, "null", viewOf("x")}), TechEngineTests::AssertFired);
    }

    SECTION("a duplicate id") {
        writer.add({PACK_MESH.id, PACK_MESH_TYPE, "first", viewOf("x")});

        REQUIRE_THROWS_AS(writer.add({PACK_MESH.id, PACK_MESH_TYPE, "second", viewOf("y")}), TechEngineTests::AssertFired);
    }

    SECTION("a duplicate id added after other entries") {
        writer.add({PACK_MESH.id, PACK_MESH_TYPE, "first", viewOf("x")});
        writer.add({PACK_SCENE.id, PACK_SCENE_TYPE, "second", viewOf("y")});

        REQUIRE_THROWS_AS(writer.add({PACK_MESH.id, PACK_MESH_TYPE, "third", viewOf("z")}), TechEngineTests::AssertFired);
    }
}
