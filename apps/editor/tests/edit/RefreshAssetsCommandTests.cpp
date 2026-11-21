#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>
#include <edit/RefreshAssetsCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetIndex;
using TechEngine::AssetRecord;
using TechEngine::ListedEntry;
using TechEngine::ListedKind;
using TechEngine::RefreshAssetsCommand;
using TechEngine::ResourceId;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view REFRESH_ASSETS_ROOT = "project://assets";
static constexpr ResourceId KEPT_SCENE_ID{0x1100000000000011};
static constexpr ResourceId COPIED_SCENE_ID{0x2200000000000022};
static constexpr ResourceId REMOVED_SCENE_ID{0x3300000000000033};

class RefreshScratch {
public:
    explicit RefreshScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_scratch.makeDirectory("assets/common");
        m_scratch.makeDirectory("assets/client");
        m_scratch.makeDirectory("assets/server");
        m_mounts.mount("project", m_scratch.root());
    }

    const ScratchDirectory& scratch() const {
        return m_scratch;
    }

    const TechEngine::FileAccess& files() const {
        return m_files;
    }

    void writeScene(const std::filesystem::path& relative, const ResourceId id) const {
        TechEngineTests::writeBytes(m_scratch, relative, TechEngineTests::assetBytes(TechEngine::SCENE_KIND, id, "scene payload"));
    }

private:
    ScratchDirectory m_scratch;
    TechEngine::MountTable m_mounts;
    TechEngine::FileAccess m_files;
};

static bool isListed(const AssetIndex& index, const std::string_view path) {
    return std::ranges::find(index.getListing(), path, &ListedEntry::path) != index.getListing().end();
}

TEST_CASE("the refresh command lists and indexes what changed on disk since the last listing", "[editor][edit][refresh][os]") {
    const RefreshScratch env{"refreshPicksUpChanges"};
    env.writeScene("assets/common/kept.tescene", KEPT_SCENE_ID);
    env.writeScene("assets/client/removed.tescene", REMOVED_SCENE_ID);
    AssetIndex index;
    REQUIRE(index.build(env.files(), REFRESH_ASSETS_ROOT) == std::error_code{});
    const std::uint64_t revision = index.getListingRevision();

    env.writeScene("assets/client/copied/copied.tescene", COPIED_SCENE_ID);
    std::filesystem::remove(env.scratch().root() / "assets" / "client" / "removed.tescene");
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    RefreshAssetsCommand command{env.files(), index};

    command.apply(scene);

    AssetRecord record;
    CHECK(index.find(KEPT_SCENE_ID, record));
    REQUIRE(index.find(COPIED_SCENE_ID, record));
    CHECK(record.side == TechEngine::Side::Client);
    CHECK(record.path == "project://assets/client/copied/copied.tescene");
    CHECK_FALSE(index.find(REMOVED_SCENE_ID, record));

    CHECK(isListed(index, "project://assets/client/copied"));
    CHECK(isListed(index, "project://assets/client/copied/copied.tescene"));
    CHECK_FALSE(isListed(index, "project://assets/client/removed.tescene"));
    CHECK(index.getListingRevision() == revision + 1);

    SECTION("applying it again lists the same entries under a new revision") {
        const std::size_t listed = index.getListing().size();

        command.apply(scene);

        CHECK(index.getListing().size() == listed);
        CHECK(index.getListingRevision() == revision + 2);
    }
}

TEST_CASE("a refresh that cannot list keeps the index and its listing and logs a warning", "[editor][edit][refresh][os]") {
    const RefreshScratch env{"refreshFailureKeeps"};
    env.writeScene("assets/common/kept.tescene", KEPT_SCENE_ID);
    AssetIndex index;
    REQUIRE(index.build(env.files(), REFRESH_ASSETS_ROOT) == std::error_code{});
    const std::size_t listed = index.getListing().size();
    const std::uint64_t revision = index.getListingRevision();

    env.writeScene("assets/common/later.tescene", COPIED_SCENE_ID);
    std::filesystem::remove_all(env.scratch().root() / "assets" / "server");
    TechEngine::ComponentRegistry registry;
    TechEngine::Scene scene(registry);
    const TechEngineTests::LogCaptureGuard logs;

    RefreshAssetsCommand{env.files(), index}.apply(scene);

    AssetRecord record;
    CHECK(index.find(KEPT_SCENE_ID, record));
    CHECK_FALSE(index.find(COPIED_SCENE_ID, record));
    CHECK(index.getListing().size() == listed);
    CHECK(index.getListingRevision() == revision);
    const std::vector<TechEngine::StoredLogRecord> records = logs.getRecords();
    const bool warned = std::ranges::any_of(records, [](const TechEngine::StoredLogRecord& logged) {
        return logged.level == TechEngine::Level::Warn;
    });
    CHECK(warned);
}
