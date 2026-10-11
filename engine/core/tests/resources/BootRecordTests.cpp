#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <system_error>
#include <vector>

using TechEngine::BOOT_ROLE_PACKS_SECTION;
using TechEngine::BOOT_SECTION_VERSION;
using TechEngine::BOOT_SIDES_SECTION;
using TechEngine::BOOT_STARTUP_SCENE_SECTION;
using TechEngine::BOOT_SYSTEM_SETTINGS_SECTION;
using TechEngine::BootRecord;
using TechEngine::ReadError;
using TechEngine::ResourceId;
using TechEngine::Role;
using TechEngine::Side;
using TechEngine::StringId;

static constexpr StringId UNKNOWN_SECTION{"Tests.Boot.Unknown"};

struct BootSection {
    StringId tag;
    std::uint16_t version = 0;
    std::size_t versionPosition = 0;
    std::size_t lengthPosition = 0;
    std::size_t payloadPosition = 0;
    std::uint32_t length = 0;
};

static BootRecord sampleRecord() {
    BootRecord record;
    record.startupScene = ResourceId(0x1000);
    record.rolePacks = {
        {Role::Client, {"common.tepack", "client.tepack"}},
        {Role::ListenServer, {"common.tepack", "client.tepack", "server.tepack"}},
        {Role::DedicatedServer, {"common.tepack", "server.tepack"}},
    };
    record.sides = {
        {ResourceId(0x1000), Side::Common},
        {ResourceId(0x2000), Side::Client},
        {ResourceId(0x4000), Side::Server},
    };
    return record;
}

static BootRecord sentinelRecord() {
    BootRecord record;
    record.startupScene = ResourceId(0x99);
    record.sides = {{ResourceId(0x99), Side::Server}};
    return record;
}

static std::vector<std::byte> writeSample() {
    std::vector<std::byte> bytes;
    sampleRecord().write(bytes);
    return bytes;
}

static std::vector<BootSection> sectionsOf(const std::vector<std::byte>& bytes) {
    TechEngine::Reader reader{bytes};
    std::uint32_t count = 0;
    reader.read(count);
    std::vector<BootSection> sections;
    for (std::uint32_t index = 0; index < count && reader.ok(); index++) {
        BootSection section;
        reader.read(section.tag);
        section.versionPosition = bytes.size() - reader.remaining();
        reader.read(section.version);
        section.lengthPosition = bytes.size() - reader.remaining();
        reader.read(section.length);
        section.payloadPosition = bytes.size() - reader.remaining();
        std::vector<std::byte> payload(section.length);
        reader.readBytes(payload);
        sections.push_back(section);
    }
    REQUIRE(reader.ok());
    REQUIRE(reader.remaining() == 0);
    return sections;
}

static BootSection sectionOf(const std::vector<std::byte>& bytes, const StringId tag) {
    for (const BootSection& section: sectionsOf(bytes)) {
        if (section.tag == tag) {
            return section;
        }
    }
    FAIL("no section with tag " << tag.value());
    return {};
}

template<typename T>
static void patch(std::vector<std::byte>& bytes, const std::size_t position, const T value) {
    REQUIRE(position + sizeof(T) <= bytes.size());
    std::memcpy(bytes.data() + position, &value, sizeof(T));
}

static void requireSameRecord(const BootRecord& actual, const BootRecord& expected) {
    CHECK(actual.startupScene == expected.startupScene);

    REQUIRE(actual.systemSettings.size() == expected.systemSettings.size());
    for (std::size_t index = 0; index < expected.systemSettings.size(); index++) {
        CHECK(actual.systemSettings[index].system == expected.systemSettings[index].system);
        CHECK(actual.systemSettings[index].settings == expected.systemSettings[index].settings);
    }

    REQUIRE(actual.rolePacks.size() == expected.rolePacks.size());
    for (std::size_t index = 0; index < expected.rolePacks.size(); index++) {
        CHECK(actual.rolePacks[index].role == expected.rolePacks[index].role);
        CHECK(actual.rolePacks[index].packs == expected.rolePacks[index].packs);
    }

    REQUIRE(actual.sides.size() == expected.sides.size());
    for (std::size_t index = 0; index < expected.sides.size(); index++) {
        CHECK(actual.sides[index].id == expected.sides[index].id);
        CHECK(actual.sides[index].side == expected.sides[index].side);
    }
}

static void requireUntouched(const BootRecord& record) {
    requireSameRecord(record, sentinelRecord());
}

static void writeUnknownSection(TechEngine::Writer& writer) {
    writer.write(UNKNOWN_SECTION);
    writer.write(static_cast<std::uint16_t>(7));
    writer.write(static_cast<std::uint32_t>(5));
    writer.write(static_cast<std::uint8_t>(0xAB));
    writer.write(static_cast<std::uint32_t>(0xDEADBEEF));
}

TEST_CASE("a boot record round-trips its startup scene, packs per role and sides", "[core][resources][boot]") {
    const std::vector<std::byte> bytes = writeSample();
    BootRecord record;

    REQUIRE(record.read(bytes) == std::error_code{});

    requireSameRecord(record, sampleRecord());
}

TEST_CASE("a boot record carries its four sections, each at version 1, the system settings one empty", "[core][resources][boot]") {
    const std::vector<std::byte> bytes = writeSample();
    const std::vector<BootSection> sections = sectionsOf(bytes);

    REQUIRE(sections.size() == 4);
    for (const StringId tag: {BOOT_STARTUP_SCENE_SECTION, BOOT_SYSTEM_SETTINGS_SECTION, BOOT_ROLE_PACKS_SECTION, BOOT_SIDES_SECTION}) {
        INFO("tag " << tag.value());
        const BootSection section = sectionOf(bytes, tag);
        CHECK(section.version == BOOT_SECTION_VERSION);
    }

    const BootSection settings = sectionOf(bytes, BOOT_SYSTEM_SETTINGS_SECTION);
    TechEngine::Reader reader{std::span<const std::byte>{bytes}.subspan(settings.payloadPosition, settings.length)};
    std::uint32_t count = 1;
    reader.read(count);
    REQUIRE(reader.ok());
    CHECK(count == 0);
    CHECK(reader.remaining() == 0);
}

TEST_CASE("a reader skips a section it does not know", "[core][resources][boot]") {
    const std::vector<std::byte> known = writeSample();
    const std::size_t knownCount = sectionsOf(known).size();
    const std::span<const std::byte> knownSections = std::span<const std::byte>{known}.subspan(sizeof(std::uint32_t));
    std::vector<std::byte> bytes;
    TechEngine::Writer writer{bytes};
    writer.write(static_cast<std::uint32_t>(knownCount + 1));

    SECTION("before the known sections") {
        writeUnknownSection(writer);
        writer.writeBytes(knownSections);
    }

    SECTION("after the known sections") {
        writer.writeBytes(knownSections);
        writeUnknownSection(writer);
    }

    BootRecord record;
    REQUIRE(record.read(bytes) == std::error_code{});

    requireSameRecord(record, sampleRecord());
}

TEST_CASE("a known section at a version the reader does not know is BadVersion", "[core][resources][boot]") {
    std::vector<std::byte> bytes = writeSample();
    const BootSection startup = sectionOf(bytes, BOOT_STARTUP_SCENE_SECTION);
    patch<std::uint16_t>(bytes, startup.versionPosition, static_cast<std::uint16_t>(BOOT_SECTION_VERSION + 1));
    BootRecord record = sentinelRecord();

    REQUIRE(record.read(bytes) == ReadError::BadVersion);

    requireUntouched(record);
}

TEST_CASE("every truncation of a boot record fails soft as Truncated", "[core][resources][boot]") {
    const std::vector<std::byte> bytes = writeSample();

    for (std::size_t length = 0; length < bytes.size(); length++) {
        INFO("truncated to " << length << " of " << bytes.size() << " bytes");
        BootRecord record = sentinelRecord();

        REQUIRE(record.read(std::span<const std::byte>{bytes}.first(length)) == ReadError::Truncated);

        requireUntouched(record);
    }
}

TEST_CASE("a corrupt known section is Malformed and leaves the record untouched", "[core][resources][boot]") {
    std::vector<std::byte> bytes;

    SECTION("a payload longer than the section's contents") {
        TechEngine::Writer writer{bytes};
        writer.write(static_cast<std::uint32_t>(1));
        writer.write(BOOT_STARTUP_SCENE_SECTION);
        writer.write(BOOT_SECTION_VERSION);
        writer.write(static_cast<std::uint32_t>(sizeof(std::uint64_t) + 1));
        writer.write(static_cast<std::uint64_t>(0x1000));
        writer.write(static_cast<std::uint8_t>(0));
    }

    SECTION("a payload shorter than the section's contents") {
        TechEngine::Writer writer{bytes};
        writer.write(static_cast<std::uint32_t>(1));
        writer.write(BOOT_STARTUP_SCENE_SECTION);
        writer.write(BOOT_SECTION_VERSION);
        writer.write(static_cast<std::uint32_t>(sizeof(std::uint64_t) - 1));
        writer.writeBytes(std::vector<std::byte>(sizeof(std::uint64_t) - 1));
    }

    SECTION("a role outside the enum") {
        bytes = writeSample();
        const BootSection rolePacks = sectionOf(bytes, BOOT_ROLE_PACKS_SECTION);
        patch<std::uint8_t>(bytes, rolePacks.payloadPosition + sizeof(std::uint32_t), 3);
    }

    SECTION("a side outside the enum") {
        bytes = writeSample();
        const BootSection sides = sectionOf(bytes, BOOT_SIDES_SECTION);
        patch<std::uint8_t>(bytes, sides.payloadPosition + sizeof(std::uint32_t) + sizeof(std::uint64_t), 3);
    }

    SECTION("a null id in the sides") {
        bytes = writeSample();
        const BootSection sides = sectionOf(bytes, BOOT_SIDES_SECTION);
        patch<std::uint64_t>(bytes, sides.payloadPosition + sizeof(std::uint32_t), 0);
    }

    BootRecord record = sentinelRecord();

    REQUIRE(record.read(bytes) == ReadError::Malformed);

    requireUntouched(record);
}

TEST_CASE("writing a null id in the sides is a programmer error", "[core][resources][boot]") {
    BootRecord record = sampleRecord();
    record.sides.push_back({ResourceId{}, Side::Client});
    std::vector<std::byte> bytes;
    const TechEngineTests::FatalAssertGuard guard;

    REQUIRE_THROWS_AS(record.write(bytes), TechEngineTests::AssertFired);
}
