#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>
#include <assets/LooseSource.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::AssetIndex;
using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::LooseSource;
using TechEngine::MountTable;
using TechEngine::ResourceId;
using TechEngine::SCENE_KIND;
using TechEngineTests::assetBytes;
using TechEngineTests::ScratchDirectory;
using TechEngineTests::writeBytes;

static constexpr ResourceId LOOSE_COMMON_ID{0x1100000000000001};
static constexpr ResourceId LOOSE_CLIENT_ID{0x2200000000000002};
static constexpr ResourceId LOOSE_SERVER_ID{0x3300000000000003};
static constexpr ResourceId LOOSE_DUPLICATED_ID{0x4400000000000004};
static constexpr ResourceId LOOSE_UNKNOWN_ID{0x5500000000000005};

class LooseScratch {
public:
    explicit LooseScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_scratch.makeDirectory("assets/common");
        m_scratch.makeDirectory("assets/client");
        m_scratch.makeDirectory("assets/server");
        m_mounts.mount("project", m_scratch.root());

        m_commonBytes = assetBytes(SCENE_KIND, LOOSE_COMMON_ID, "common scene");
        m_clientBytes = assetBytes(SCENE_KIND, LOOSE_CLIENT_ID, "client scene, a little longer than the header");
        writeBytes(m_scratch, "assets/common/level.tescene", m_commonBytes);
        writeBytes(m_scratch, "assets/client/menu.tescene", m_clientBytes);
        writeBytes(m_scratch, "assets/server/arena.tescene", assetBytes(SCENE_KIND, LOOSE_SERVER_ID, "server scene"));
        writeBytes(m_scratch, "assets/common/a.tescene", assetBytes(SCENE_KIND, LOOSE_DUPLICATED_ID, "a"));
        writeBytes(m_scratch, "assets/client/b.tescene", assetBytes(SCENE_KIND, LOOSE_DUPLICATED_ID, "b"));

        REQUIRE(m_index.build(m_files, "project://assets") == std::error_code{});
    }

    const ScratchDirectory& scratch() const {
        return m_scratch;
    }

    const FileAccess& files() const {
        return m_files;
    }

    const AssetIndex& index() const {
        return m_index;
    }

    const std::vector<std::byte>& commonBytes() const {
        return m_commonBytes;
    }

    const std::vector<std::byte>& clientBytes() const {
        return m_clientBytes;
    }

private:
    ScratchDirectory m_scratch;
    MountTable m_mounts;
    FileAccess m_files;
    AssetIndex m_index;
    std::vector<std::byte> m_commonBytes;
    std::vector<std::byte> m_clientBytes;
};

TEST_CASE("the loose source returns the bytes of a common or client id", "[editor][assets]") {
    const LooseScratch env{"looseSourcePresent"};
    const LooseSource source{env.files(), env.index()};

    CHECK(source.find(LOOSE_COMMON_ID) == AssetAvailability::Present);
    CHECK(source.find(LOOSE_CLIENT_ID) == AssetAvailability::Present);

    std::vector<std::byte> common;
    REQUIRE(source.read(LOOSE_COMMON_ID, common) == std::error_code{});
    CHECK(common == env.commonBytes());

    std::vector<std::byte> client;
    REQUIRE(source.read(LOOSE_CLIENT_ID, client) == std::error_code{});
    CHECK(client == env.clientBytes());
}

TEST_CASE("the loose source reports a server id as absent for the client role", "[editor][assets]") {
    const LooseScratch env{"looseSourceServer"};
    const LooseSource source{env.files(), env.index()};

    CHECK(source.find(LOOSE_SERVER_ID) == AssetAvailability::AbsentForRole);
}

TEST_CASE("the loose source reports an id the index does not hold as unknown", "[editor][assets]") {
    const LooseScratch env{"looseSourceUnknown"};
    const LooseSource source{env.files(), env.index()};

    CHECK(source.find(LOOSE_UNKNOWN_ID) == AssetAvailability::Unknown);
    CHECK(source.find(ResourceId()) == AssetAvailability::Unknown);
    CHECK(source.find(LOOSE_DUPLICATED_ID) == AssetAvailability::Unknown);
}

TEST_CASE("the loose source passes a file error through from read", "[editor][assets]") {
    const LooseScratch env{"looseSourceFileError"};
    const LooseSource source{env.files(), env.index()};
    std::filesystem::remove(env.scratch().root() / "assets" / "client" / "menu.tescene");

    std::vector<std::byte> out;
    CHECK(source.read(LOOSE_CLIENT_ID, out) == FileError::NotFound);
}

TEST_CASE("reading an id the loose source does not hold as Present is a fatal check", "[editor][assets]") {
    const LooseScratch env{"looseSourceNotPresent"};
    const LooseSource source{env.files(), env.index()};
    const TechEngineTests::FatalAssertGuard guard;
    std::vector<std::byte> out;

    SECTION("a server id") {
        REQUIRE_THROWS_AS(source.read(LOOSE_SERVER_ID, out), TechEngineTests::AssertFired);
    }

    SECTION("an unknown id") {
        REQUIRE_THROWS_AS(source.read(LOOSE_UNKNOWN_ID, out), TechEngineTests::AssertFired);
    }
}
