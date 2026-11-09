#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImport.hpp>
#include <assets/ModelTestFiles.hpp>
#include <edit/ImportModelCommand.hpp>

#include <catch2/catch_test_macros.hpp>

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
using TechEngine::RegisterImportedAssetsCommand;
using TechEngine::ResourceId;
using TechEngineTests::AppTestAccess;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view PAIR_SOURCE = "project://assets/client/Pair.gltf";
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
        scratch.makeDirectory("assets/client");
    }

    const TechEngine::FileAccess& files() const {
        return m_files;
    }

    ImportModelCommand makeImport(const std::string_view sourcePath) {
        TechEngine::ModelImportRequest request;
        request.sourcePath = std::string(sourcePath);
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

static AssetRecord makeRecord(const ResourceId id, const std::string_view path) {
    AssetRecord record;
    record.id = id;
    record.typeTag = TechEngine::MESH_TYPE_TAG;
    record.side = TechEngine::Side::Client;
    record.path = std::string(path);
    return record;
}

TEST_CASE("registering imported assets adds each record to the index", "[editor][edit][import]") {
    ImportAppProbe app{"importRegisterAdds"};
    constexpr ResourceId first{0x20};
    constexpr ResourceId second{0x10};
    RegisterImportedAssetsCommand command{app.index, {makeRecord(first, "project://assets/client/A.temesh"), makeRecord(second, "project://assets/client/B.temesh")}};

    command.apply(AppTestAccess::scene(app));

    REQUIRE(app.index.getRecords().size() == 2);
    AssetRecord found;
    REQUIRE(app.index.find(first, found));
    CHECK(found.path == "project://assets/client/A.temesh");
    CHECK(found.typeTag == TechEngine::MESH_TYPE_TAG);
    CHECK(found.side == TechEngine::Side::Client);
    REQUIRE(app.index.find(second, found));
    CHECK(found.path == "project://assets/client/B.temesh");

    SECTION("applying it again adds nothing") {
        command.apply(AppTestAccess::scene(app));
        CHECK(app.index.getRecords().size() == 2);
    }
}

TEST_CASE("registering skips a record whose id the index already holds and adds the rest", "[editor][edit][import]") {
    ImportAppProbe app{"importRegisterSkipsTaken"};
    constexpr ResourceId taken{0x30};
    constexpr ResourceId fresh{0x31};
    app.index.add(makeRecord(taken, "project://assets/client/Old.temesh"));
    RegisterImportedAssetsCommand command{app.index, {makeRecord(taken, "project://assets/client/New.temesh"), makeRecord(fresh, "project://assets/client/Fresh.temesh")}};

    command.apply(AppTestAccess::scene(app));

    REQUIRE(app.index.getRecords().size() == 2);
    AssetRecord found;
    REQUIRE(app.index.find(taken, found));
    CHECK(found.path == "project://assets/client/Old.temesh");
    REQUIRE(app.index.find(fresh, found));
    CHECK(found.path == "project://assets/client/Fresh.temesh");
}

TEST_CASE("the import command returns before the job is done and writes the files on a worker", "[editor][edit][import]") {
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

    SECTION("a second import of the same source is refused by its job and changes no file") {
        app.makeImport(PAIR_SOURCE).apply(AppTestAccess::scene(app));
        app.waitForImports();

        std::vector<std::byte> after;
        REQUIRE(app.files().read(PAIR_MODEL, after) == std::error_code{});
        CHECK(after == bytes);
    }
}

TEST_CASE("an import command whose job fails writes nothing and registers nothing", "[editor][edit][import]") {
    ImportAppProbe app{"importCommandFails"};

    app.makeImport("project://assets/client/Missing.gltf").apply(AppTestAccess::scene(app));
    app.waitForImports();

    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Missing"));
    CHECK(app.index.getRecords().empty());
}

TEST_CASE("a running simulation adds the imported model and its meshes to the index", "[editor][edit][import][integration]") {
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
    REQUIRE(records.size() == 3);

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
}
