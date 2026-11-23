#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ModelTestFiles.hpp>
#include <edit/ImportModelCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::ImportModelCommand;
using TechEngine::ListedEntry;
using TechEngine::ListedKind;
using TechEngineTests::AppTestAccess;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view IMPORT_ASSETS_ROOT = "project://assets";
static constexpr std::string_view PAIR_SOURCE = "project://assets/client/Pair.gltf";
static constexpr std::string_view PAIR_FOLDER = "project://assets/client/Pair";
static constexpr std::string_view PAIR_MODEL = "project://assets/client/Pair/Pair.temodel";

class ImportAppProbe final : public TechEngine::App {
public:
    ScratchDirectory scratch;
    TechEngine::AssetIndex index;
    std::vector<TechEngine::BatchId> batches;
    bool importAtStartup = false;
    std::atomic<bool> registered = false;

    explicit ImportAppProbe(const std::string_view name) : App(TechEngine::Role::DedicatedServer), scratch(name) {
        m_mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/common");
        scratch.makeDirectory("assets/client");
        scratch.makeDirectory("assets/server");
    }

    const TechEngine::FileAccess& files() const {
        return m_files;
    }

    ImportModelCommand makeImport(const std::string_view sourcePath, const std::string_view name = {}) {
        TechEngine::ModelImportRequest request;
        request.sourcePath = std::string(sourcePath);
        request.name = std::string(name);
        request.side = TechEngine::Side::Client;
        return ImportModelCommand{*this, m_jobs, m_files, index, batches, std::move(request)};
    }

    void waitForImports() {
        for (const TechEngine::BatchId batch: batches) {
            m_jobs.wait(batch);
        }
    }

protected:
    void init() override {
        REQUIRE(index.build(m_files, IMPORT_ASSETS_ROOT) == std::error_code{});
    }

    std::error_code simulationStartupLoad(TechEngine::Scene& scene) override {
        if (importAtStartup) {
            makeImport(PAIR_SOURCE).apply(scene);
        }
        return {};
    }

    // Runs on the simulation thread after each batch of commands, which is the only thread that may read the index.
    void publishEdits(const TechEngine::Scene&) override {
        if (!index.getRecords().empty()) {
            registered.store(true);
        }
    }
};

TEST_CASE("the import command returns before the job is done and writes the files on a worker", "[editor][edit][import][os]") {
    ImportAppProbe app{"importCommandWritesFiles"};
    app.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    ImportModelCommand command = app.makeImport(PAIR_SOURCE);

    command.apply(AppTestAccess::scene(app));

    REQUIRE(app.batches.size() == 1);
    CHECK(app.batches[0].valid());
    app.waitForImports();

    std::vector<std::byte> bytes;
    REQUIRE(app.files().read(PAIR_MODEL, bytes) == std::error_code{});
    TechEngine::Model model;
    REQUIRE(model.read(bytes) == std::error_code{});
    CHECK(model.id.valid());
    CHECK(model.nodes.size() == 4);
    CHECK(std::filesystem::is_regular_file(app.scratch.root() / "assets" / "client" / "Pair" / "Meshes" / "Hull.temesh"));
    CHECK(std::filesystem::is_regular_file(app.scratch.root() / "assets" / "client" / "Pair" / "Meshes" / "Tire.temesh"));

    SECTION("with no simulation running, the index entries are refused and the index stays as it was") {
        CHECK(app.index.getRecords().empty());
    }

    SECTION("a second create of the same source writes the next folder and changes no file of the first") {
        app.makeImport(PAIR_SOURCE).apply(AppTestAccess::scene(app));
        app.waitForImports();

        std::vector<std::byte> after;
        REQUIRE(app.files().read(PAIR_MODEL, after) == std::error_code{});
        CHECK(after == bytes);
        std::vector<std::byte> secondBytes;
        REQUIRE(app.files().read("project://assets/client/Pair_1/Pair_1.temodel", secondBytes) == std::error_code{});
        TechEngine::Model second;
        REQUIRE(second.read(secondBytes) == std::error_code{});
        CHECK(second.id != model.id);
    }
}

TEST_CASE("an import command with an invalid name starts no job and writes nothing", "[editor][edit][import][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    ImportAppProbe app{"importCommandRefusesName"};
    app.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    app.makeImport(PAIR_SOURCE, "Karts/Pair").apply(AppTestAccess::scene(app));
    app.makeImport(PAIR_SOURCE, "Pair.").apply(AppTestAccess::scene(app));

    CHECK(app.batches.empty());
    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Pair"));
    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Karts"));
    const auto refusals = std::ranges::count_if(logs.getMessages(), [](const std::string& message) {
        return message.find("cannot be named") != std::string::npos;
    });
    CHECK(refusals == 2);
}

TEST_CASE("an import command whose job fails writes nothing and registers nothing", "[editor][edit][import][os]") {
    ImportAppProbe app{"importCommandFails"};

    app.makeImport("project://assets/client/Missing.gltf").apply(AppTestAccess::scene(app));
    app.waitForImports();

    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Missing"));
    CHECK(app.index.getRecords().empty());
}

TEST_CASE("a finished create lists its folder and indexes the model, its meshes and its material", "[editor][edit][import][integration][os]") {
    ImportAppProbe app{"importCommandRegisters"};
    app.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    app.importAtStartup = true;

    std::atomic<bool> timedOut = false;
    std::jthread controller{[&app, &timedOut] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
        while (!app.registered.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        timedOut = !app.registered.load();
        app.requestStop();
    }};
    const int result = app.run();
    controller.join();
    app.waitForImports();

    CHECK(result == 0);
    REQUIRE_FALSE(timedOut.load());
    const std::vector<AssetRecord>& records = app.index.getRecords();
    REQUIRE(records.size() == 4);

    std::vector<std::byte> bytes;
    REQUIRE(app.files().read(PAIR_MODEL, bytes) == std::error_code{});
    TechEngine::Model model;
    REQUIRE(model.read(bytes) == std::error_code{});
    AssetRecord modelRecord;
    REQUIRE(app.index.find(model.id, modelRecord));
    CHECK(modelRecord.typeTag == TechEngine::MODEL_TYPE_TAG);
    CHECK(modelRecord.side == TechEngine::Side::Client);
    CHECK(modelRecord.path == PAIR_MODEL);

    std::size_t meshNodes = 0;
    for (const TechEngine::ModelNode& node: model.nodes) {
        if (!node.mesh.valid()) {
            continue;
        }
        meshNodes++;
        AssetRecord meshRecord;
        REQUIRE(app.index.find(node.mesh, meshRecord));
        CHECK(meshRecord.typeTag == TechEngine::MESH_TYPE_TAG);
        CHECK(meshRecord.side == TechEngine::Side::Client);
    }
    CHECK(meshNodes == 2);

    const auto material = std::ranges::find(records, TechEngine::MATERIAL_TYPE_TAG, &AssetRecord::typeTag);
    REQUIRE(material != records.end());
    CHECK(material->path == "project://assets/client/Pair/Materials/Material 0.tematerial");

    const std::vector<ListedEntry>& listing = app.index.getListing();
    const auto folder = std::ranges::find(listing, PAIR_FOLDER, &ListedEntry::path);
    REQUIRE(folder != listing.end());
    CHECK(folder->kind == ListedKind::Folder);
    CHECK(folder->side == TechEngine::Side::Client);
    const auto listedModel = std::ranges::find(listing, PAIR_MODEL, &ListedEntry::path);
    REQUIRE(listedModel != listing.end());
    CHECK(listedModel->kind == ListedKind::Asset);
    CHECK(listedModel->id == model.id);
    CHECK(listedModel->typeTag == TechEngine::MODEL_TYPE_TAG);
    const auto listedSource = std::ranges::find(listing, PAIR_SOURCE, &ListedEntry::path);
    REQUIRE(listedSource != listing.end());
    CHECK(listedSource->kind == ListedKind::ModelSource);
    CHECK(app.index.getListingRevision() == 2);
}
