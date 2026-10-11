#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/assets/PackSource.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/PackWriter.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::AssetSource;
using TechEngine::BootRecord;
using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::MountTable;
using TechEngine::PackSource;
using TechEngine::ReadError;
using TechEngine::ResourceId;
using TechEngine::Role;
using TechEngine::Side;
using TechEngine::StringId;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view SOURCE_DIRECTORY = "packs://";

static constexpr StringId SOURCE_SCENE_TYPE{"Tests.SourceScene"};
static constexpr StringId SOURCE_MESH_TYPE{"Tests.SourceMesh"};

struct SourceSampleEntry {
    ResourceId id;
    StringId typeTag;
    std::string_view payload;
};

static constexpr SourceSampleEntry COMMON_SCENE{ResourceId(0x1000), SOURCE_SCENE_TYPE, "common scene"};
static constexpr SourceSampleEntry CLIENT_MESH{ResourceId(0x2000), SOURCE_MESH_TYPE, "client mesh, a little longer than sixteen"};
static constexpr SourceSampleEntry CLIENT_TEXTURE{ResourceId(0x3000), SOURCE_MESH_TYPE, "t"};
static constexpr SourceSampleEntry SERVER_NAVMESH{ResourceId(0x4000), SOURCE_MESH_TYPE, "server navmesh"};

static constexpr ResourceId UNKNOWN_ID{0x5000};

class PackSourceScratch {
public:
    explicit PackSourceScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_mounts.mount("packs", m_scratch.root());
    }

    FileAccess& files() {
        return m_files;
    }

    void storePack(const std::string_view fileName, const std::initializer_list<SourceSampleEntry> entries) {
        TechEngine::PackWriter writer;
        for (const SourceSampleEntry& entry: entries) {
            writer.add({entry.id, entry.typeTag, fileName, viewOf(entry.payload)});
        }
        std::vector<std::byte> bytes;
        writer.write(bytes);
        REQUIRE(m_files.write(pathOf(fileName), bytes) == std::error_code{});
    }

    void storeSamplePacks() {
        storePack("common.tepack", {COMMON_SCENE});
        storePack("client.tepack", {CLIENT_TEXTURE, CLIENT_MESH});
        storePack("server.tepack", {SERVER_NAVMESH});
    }

    static std::string pathOf(const std::string_view fileName) {
        return std::string(SOURCE_DIRECTORY) + std::string(fileName);
    }

    static std::span<const std::byte> viewOf(const std::string_view text) {
        return std::as_bytes(std::span<const char>{text.data(), text.size()});
    }

private:
    ScratchDirectory m_scratch;
    MountTable m_mounts;
    FileAccess m_files;
};

static std::vector<std::byte> bytesOf(const std::string_view text) {
    const std::span<const std::byte> view = PackSourceScratch::viewOf(text);
    return {view.begin(), view.end()};
}

static BootRecord sampleRecord() {
    BootRecord record;
    record.startupScene = COMMON_SCENE.id;
    record.rolePacks = {
        {Role::Client, {"common.tepack", "client.tepack"}},
        {Role::DedicatedServer, {"common.tepack", "server.tepack"}},
    };
    record.sides = {
        {COMMON_SCENE.id, Side::Common},
        {CLIENT_MESH.id, Side::Client},
        {CLIENT_TEXTURE.id, Side::Client},
        {SERVER_NAVMESH.id, Side::Server},
    };
    return record;
}

static void requireReads(const AssetSource& source, const SourceSampleEntry& sample) {
    INFO("id " << sample.id.value());
    REQUIRE(source.find(sample.id) == AssetAvailability::Present);
    std::vector<std::byte> out;
    REQUIRE(source.read(sample.id, out) == std::error_code{});
    CHECK(out == bytesOf(sample.payload));
}

TEST_CASE("two packs merge into one table for the role that opens them", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceMerge"};
    scratch.storeSamplePacks();
    PackSource packSource{scratch.files()};

    REQUIRE(packSource.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == std::error_code{});

    const AssetSource& source = packSource;
    for (const SourceSampleEntry& sample: {COMMON_SCENE, CLIENT_MESH, CLIENT_TEXTURE}) {
        requireReads(source, sample);
    }
}

TEST_CASE("an id on a side the role never opens is absent for that role", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceAbsent"};
    scratch.storeSamplePacks();
    PackSource source{scratch.files()};

    SECTION("a server id for the client") {
        REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == std::error_code{});

        CHECK(source.find(SERVER_NAVMESH.id) == AssetAvailability::AbsentForRole);
    }

    SECTION("client ids for the dedicated server") {
        REQUIRE(source.open(sampleRecord(), Role::DedicatedServer, SOURCE_DIRECTORY) == std::error_code{});

        CHECK(source.find(CLIENT_MESH.id) == AssetAvailability::AbsentForRole);
        CHECK(source.find(CLIENT_TEXTURE.id) == AssetAvailability::AbsentForRole);
        requireReads(source, COMMON_SCENE);
        requireReads(source, SERVER_NAVMESH);
    }
}

TEST_CASE("an id in no pack and no side is unknown", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceUnknown"};
    scratch.storeSamplePacks();
    PackSource source{scratch.files()};

    CHECK(source.find(COMMON_SCENE.id) == AssetAvailability::Unknown);

    REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == std::error_code{});

    CHECK(source.find(UNKNOWN_ID) == AssetAvailability::Unknown);
    CHECK(source.find(ResourceId{}) == AssetAvailability::Unknown);
}

TEST_CASE("the pack source reads an id's bytes from its pack when asked, not when it opens", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceReadLate"};
    scratch.storeSamplePacks();
    PackSource source{scratch.files()};
    REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == std::error_code{});

    REQUIRE(scratch.files().remove(PackSourceScratch::pathOf("client.tepack"), false) == std::error_code{});

    std::vector<std::byte> out = bytesOf("untouched");
    REQUIRE(source.read(CLIENT_MESH.id, out) == FileError::NotFound);
    CHECK(out == bytesOf("untouched"));
    requireReads(source, COMMON_SCENE);
}

TEST_CASE("the same id in two of a role's packs is Malformed and leaves the source empty", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceDuplicate"};
    scratch.storePack("common.tepack", {COMMON_SCENE, CLIENT_MESH});
    scratch.storePack("client.tepack", {CLIENT_MESH});
    PackSource source{scratch.files()};

    REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == ReadError::Malformed);

    CHECK(source.find(COMMON_SCENE.id) == AssetAvailability::Unknown);
    CHECK(source.find(CLIENT_MESH.id) == AssetAvailability::Unknown);
    CHECK(source.find(SERVER_NAVMESH.id) == AssetAvailability::Unknown);
}

TEST_CASE("a missing pack is reported with the file error unchanged and leaves the source empty", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceMissing"};
    scratch.storePack("common.tepack", {COMMON_SCENE});
    PackSource source{scratch.files()};

    REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == FileError::NotFound);

    CHECK(source.find(COMMON_SCENE.id) == AssetAvailability::Unknown);
    CHECK(source.find(SERVER_NAVMESH.id) == AssetAvailability::Unknown);
}

TEST_CASE("a role the boot record does not list is Malformed and leaves the source empty", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceUnlistedRole"};
    scratch.storeSamplePacks();
    PackSource source{scratch.files()};

    REQUIRE(source.open(sampleRecord(), Role::ListenServer, SOURCE_DIRECTORY) == ReadError::Malformed);

    CHECK(source.find(COMMON_SCENE.id) == AssetAvailability::Unknown);
    CHECK(source.find(SERVER_NAVMESH.id) == AssetAvailability::Unknown);
}

TEST_CASE("reading an id that is not present is a programmer error", "[core][resources][packsource]") {
    PackSourceScratch scratch{"packSourceNotPresent"};
    scratch.storeSamplePacks();
    PackSource source{scratch.files()};
    REQUIRE(source.open(sampleRecord(), Role::Client, SOURCE_DIRECTORY) == std::error_code{});
    const TechEngineTests::FatalAssertGuard guard;
    std::vector<std::byte> out;

    SECTION("absent for the role") {
        REQUIRE_THROWS_AS(source.read(SERVER_NAVMESH.id, out), TechEngineTests::AssertFired);
    }

    SECTION("unknown") {
        REQUIRE_THROWS_AS(source.read(UNKNOWN_ID, out), TechEngineTests::AssertFired);
    }
}
