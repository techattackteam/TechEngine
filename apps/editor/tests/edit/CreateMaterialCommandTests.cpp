#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/CreateMaterialCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetIndex;
using TechEngine::AssetRecord;
using TechEngine::CreateMaterialCommand;
using TechEngine::ListedEntry;
using TechEngine::MaterialCreateRequest;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view MATERIAL_ASSETS_ROOT = "project://assets";
static constexpr std::string_view CLIENT_FOLDER = "project://assets/client";

class MaterialScratch {
public:
    AssetIndex index;

    explicit MaterialScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_scratch.makeDirectory("assets/common");
        m_scratch.makeDirectory("assets/client");
        m_scratch.makeDirectory("assets/server");
        m_mounts.mount("project", m_scratch.root());
    }

    const ScratchDirectory& scratch() const {
        return m_scratch;
    }

    TechEngine::FileAccess& files() {
        return m_files;
    }

    void buildIndex() {
        REQUIRE(index.build(m_files, MATERIAL_ASSETS_ROOT) == std::error_code{});
    }

    void create(const std::string_view folder, const std::string_view name) {
        MaterialCreateRequest request;
        request.folder = std::string(folder);
        request.name = std::string(name);
        CreateMaterialCommand command{m_files, index, std::move(request)};
        command.apply(m_scene);
    }

    std::size_t materialCount() const {
        return static_cast<std::size_t>(std::ranges::count(index.getRecords(), TechEngine::MATERIAL_TYPE_TAG, &AssetRecord::typeTag));
    }

    bool findByPath(const std::string_view path, AssetRecord& out) const {
        const auto found = std::ranges::find(index.getRecords(), path, &AssetRecord::path);
        if (found == index.getRecords().end()) {
            return false;
        }
        out = *found;
        return true;
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

static bool isListed(const AssetIndex& index, const std::string_view path) {
    return std::ranges::find(index.getListing(), path, &ListedEntry::path) != index.getListing().end();
}

TEST_CASE("a material's output path is its folder, its name, the suffix and the material extension", "[editor][edit][material]") {
    CHECK(CreateMaterialCommand::outputPath(CLIENT_FOLDER, "Brick", 0) == underClient("Brick.tematerial"));
    CHECK(CreateMaterialCommand::outputPath(CLIENT_FOLDER, "Brick", 1) == underClient("Brick_1.tematerial"));
    CHECK(CreateMaterialCommand::outputPath(CLIENT_FOLDER, "Brick", 12) == underClient("Brick_12.tematerial"));
    CHECK(CreateMaterialCommand::outputPath(CLIENT_FOLDER, "Old.Brick", 1) == underClient("Old.Brick_1.tematerial"));
    CHECK(CreateMaterialCommand::outputPath(underClient("Walls/Inner"), "Brick", 0) == underClient("Walls/Inner/Brick.tematerial"));
}

TEST_CASE("a created material is a default material under a new id, indexed and listed at once", "[editor][edit][material][os]") {
    MaterialScratch env{"materialCreatesDefault"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(CLIENT_FOLDER, "Brick");

    AssetRecord record;
    REQUIRE(env.findByPath(underClient("Brick.tematerial"), record));
    CHECK(record.typeTag == TechEngine::MATERIAL_TYPE_TAG);
    CHECK(record.side == TechEngine::Side::Client);
    CHECK(record.id.valid());
    CHECK(isListed(env.index, underClient("Brick.tematerial")));
    CHECK(env.index.getListingRevision() == revision + 1);

    std::vector<std::byte> bytes;
    REQUIRE(env.files().read(underClient("Brick.tematerial"), bytes) == std::error_code{});
    TechEngine::Material material;
    REQUIRE(material.read(bytes) == std::error_code{});
    CHECK(material.id == record.id);
    TechEngine::Material expected;
    expected.id = record.id;
    CHECK(material == expected);
}

TEST_CASE("a material whose name is taken takes the first free suffix, and each create has its own id", "[editor][edit][material][os]") {
    MaterialScratch env{"materialTakesNextName"};
    env.buildIndex();

    env.create(CLIENT_FOLDER, "Brick");
    env.create(CLIENT_FOLDER, "Brick");
    env.create(CLIENT_FOLDER, "Brick");

    AssetRecord first;
    AssetRecord second;
    AssetRecord third;
    REQUIRE(env.findByPath(underClient("Brick.tematerial"), first));
    REQUIRE(env.findByPath(underClient("Brick_1.tematerial"), second));
    REQUIRE(env.findByPath(underClient("Brick_2.tematerial"), third));
    CHECK(first.id != second.id);
    CHECK(second.id != third.id);
    CHECK(first.id != third.id);
    CHECK(env.materialCount() == 3);

    SECTION("a gap is filled first") {
        std::filesystem::remove(env.scratch().root() / "assets" / "client" / "Brick_1.tematerial");

        env.create(CLIENT_FOLDER, "Brick");

        AssetRecord refilled;
        REQUIRE(env.findByPath(underClient("Brick_1.tematerial"), refilled));
        CHECK(refilled.id != second.id);
        CHECK_FALSE(std::filesystem::exists(env.scratch().root() / "assets" / "client" / "Brick_3.tematerial"));
    }
}

TEST_CASE("a file that is not a material still takes the name, and is left as it was", "[editor][edit][material][os]") {
    MaterialScratch env{"materialSkipsForeignFile"};
    env.scratch().writeFile("assets/client/Brick.tematerial", "not a material");
    env.buildIndex();

    env.create(CLIENT_FOLDER, "Brick");

    AssetRecord record;
    REQUIRE(env.findByPath(underClient("Brick_1.tematerial"), record));
    CHECK(record.typeTag == TechEngine::MATERIAL_TYPE_TAG);
    std::vector<std::byte> bytes;
    REQUIRE(env.files().read(underClient("Brick.tematerial"), bytes) == std::error_code{});
    CHECK(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()) == "not a material");
}

TEST_CASE("a material created in a subfolder is indexed with its side", "[editor][edit][material][os]") {
    MaterialScratch env{"materialInSubfolder"};
    env.scratch().makeDirectory("assets/common/Walls");
    env.buildIndex();

    env.create("project://assets/common/Walls", "Brick");

    AssetRecord record;
    REQUIRE(env.findByPath("project://assets/common/Walls/Brick.tematerial", record));
    CHECK(record.side == TechEngine::Side::Common);
}

TEST_CASE("an invalid material name writes nothing and leaves the listing as it was", "[editor][edit][material][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    MaterialScratch env{"materialRefusesName"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(CLIENT_FOLDER, "");
    env.create(CLIENT_FOLDER, "Walls/Brick");
    env.create(CLIENT_FOLDER, "Brick.");

    CHECK(env.materialCount() == 0);
    CHECK(env.index.getListingRevision() == revision);
    CHECK(std::filesystem::is_empty(env.scratch().root() / "assets" / "client"));
    const auto refusals = std::ranges::count_if(logs.getMessages(), [](const std::string& message) {
        return message.find("cannot be named") != std::string::npos;
    });
    CHECK(refusals == 3);
}

TEST_CASE("a material in a folder that is gone writes nothing and logs the failure", "[editor][edit][material][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    MaterialScratch env{"materialMissingFolder"};
    env.buildIndex();
    const std::uint64_t revision = env.index.getListingRevision();

    env.create(underClient("Gone"), "Brick");

    CHECK(env.materialCount() == 0);
    CHECK(env.index.getListingRevision() == revision);
    CHECK(std::ranges::any_of(logs.getMessages(), [](const std::string& message) {
        return message.find("Failed to create the material") != std::string::npos;
    }));
}
