#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/assets/CreateFolderCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

using TechEngine::AssetIndex;
using TechEngine::CreateFolderCommand;
using TechEngine::FolderCreateRequest;
using TechEngine::ListedEntry;
using TechEngine::ListedKind;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view FOLDER_ASSETS_ROOT = "project://assets";
static constexpr std::string_view CLIENT_FOLDER = "project://assets/client";

class FolderScratch {
public:
    AssetIndex index;

    explicit FolderScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_scratch.makeDirectory("assets/common");
        m_scratch.makeDirectory("assets/client");
        m_scratch.makeDirectory("assets/server");
        m_mounts.mount("project", m_scratch.root());
    }

    const ScratchDirectory& scratch() const {
        return m_scratch;
    }

    void buildIndex() {
        REQUIRE(index.build(m_files, FOLDER_ASSETS_ROOT) == std::error_code{});
    }

    void create(const std::string_view folder, const std::string_view name) {
        FolderCreateRequest request;
        request.folder = std::string(folder);
        request.name = std::string(name);
        CreateFolderCommand command{m_files, index, std::move(request)};
        command.apply(m_scene);
    }

    bool isListedFolder(const std::string_view path) const {
        return std::ranges::any_of(index.getListing(), [path](const ListedEntry& entry) {
            return entry.path == path && entry.kind == ListedKind::Folder;
        });
    }

private:
    ScratchDirectory m_scratch;
    TechEngine::MountTable m_mounts;
    TechEngine::FileAccess m_files;
    TechEngine::ComponentRegistry m_registry;
    TechEngine::Scene m_scene{m_registry};
};

static std::string underClient(const std::string_view relative) {
    return std::string(CLIENT_FOLDER) + "/" + std::string(relative);
}

TEST_CASE("a folder's output path is its parent, its name and the suffix", "[editor][edit][folder]") {
    CHECK(CreateFolderCommand::outputPath(CLIENT_FOLDER, "Walls", 0) == underClient("Walls"));
    CHECK(CreateFolderCommand::outputPath(CLIENT_FOLDER, "Walls", 1) == underClient("Walls_1"));
    CHECK(CreateFolderCommand::outputPath(underClient("Walls"), "Inner.v2", 3) == underClient("Walls/Inner.v2_3"));
}

TEST_CASE("a created folder is empty and listed at once", "[editor][edit][folder][os]") {
    FolderScratch env{"folderCreates"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(CLIENT_FOLDER, "Walls");

    const std::filesystem::path created = env.scratch().root() / "assets" / "client" / "Walls";
    CHECK(std::filesystem::is_directory(created));
    CHECK(std::filesystem::is_empty(created));
    CHECK(env.isListedFolder(underClient("Walls")));
    CHECK(env.index.getListingRevision() == revision + 1);
}

TEST_CASE("a folder whose name is taken takes the first free suffix, also when a file holds the name", "[editor][edit][folder][os]") {
    FolderScratch env{"folderTakesNextName"};
    env.scratch().makeDirectory("assets/client/Walls");
    env.scratch().writeFile("assets/client/Walls_1", "a file, not a folder");
    env.buildIndex();

    env.create(CLIENT_FOLDER, "Walls");
    env.create(CLIENT_FOLDER, "Walls");

    CHECK(env.isListedFolder(underClient("Walls_2")));
    CHECK(env.isListedFolder(underClient("Walls_3")));
    CHECK(std::filesystem::is_regular_file(env.scratch().root() / "assets" / "client" / "Walls_1"));
}

TEST_CASE("a folder is created inside a subfolder", "[editor][edit][folder][os]") {
    FolderScratch env{"folderInSubfolder"};
    env.scratch().makeDirectory("assets/common/Walls");
    env.buildIndex();

    env.create("project://assets/common/Walls", "Inner");

    CHECK(env.isListedFolder("project://assets/common/Walls/Inner"));
}

TEST_CASE("an invalid folder name creates nothing and leaves the listing as it was", "[editor][edit][folder][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    FolderScratch env{"folderRefusesName"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(CLIENT_FOLDER, "");
    env.create(CLIENT_FOLDER, "Walls/Inner");
    env.create(CLIENT_FOLDER, "Walls ");

    CHECK(std::filesystem::is_empty(env.scratch().root() / "assets" / "client"));
    CHECK(env.index.getListingRevision() == revision);
    const auto refusals = std::ranges::count_if(logs.getMessages(), [](const std::string& message) {
        return message.find("cannot be named") != std::string::npos;
    });
    CHECK(refusals == 3);
}

TEST_CASE("a parent folder that is gone is not brought back", "[editor][edit][folder][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    FolderScratch env{"folderMissingParent"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(underClient("Gone"), "Walls");

    CHECK_FALSE(std::filesystem::exists(env.scratch().root() / "assets" / "client" / "Gone"));
    CHECK(env.index.getListingRevision() == revision);
    CHECK(std::ranges::any_of(logs.getMessages(), [](const std::string& message) {
        return message.find("is not a folder any more") != std::string::npos;
    }));
}

TEST_CASE("a parent that is a file is refused", "[editor][edit][folder][os]") {
    FolderScratch env{"folderParentIsFile"};
    env.scratch().writeFile("assets/client/Walls", "a file");
    env.buildIndex();

    env.create(underClient("Walls"), "Inner");

    CHECK(std::filesystem::is_regular_file(env.scratch().root() / "assets" / "client" / "Walls"));
    CHECK_FALSE(env.isListedFolder(underClient("Walls/Inner")));
}
