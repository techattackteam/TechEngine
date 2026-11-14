#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ImageTestFiles.hpp>
#include <assets/TextureImporter.hpp>
#include <edit/ImportSkyCommand.hpp>

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
using TechEngine::ImportSkyCommand;
using TechEngineTests::AppTestAccess;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view SKY_SOURCE = "project://assets/client/Sky.hdr";
static constexpr std::string_view SKY_TEXTURE = "project://assets/client/Sky.tetexture";

class ImportSkyAppProbe final : public TechEngine::App {
public:
    ScratchDirectory scratch;
    TechEngine::AssetIndex index;
    std::vector<TechEngine::BatchId> batches;
    bool importAtStartup = false;
    std::atomic<bool> registered = false;

    explicit ImportSkyAppProbe(const std::string_view name) : App(TechEngine::Role::DedicatedServer), scratch(name) {
        m_mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    const TechEngine::FileAccess& files() const {
        return m_files;
    }

    ImportSkyCommand makeImport(const std::string_view sourcePath) {
        TechEngine::SkyImportRequest request;
        request.sourcePath = std::string(sourcePath);
        return ImportSkyCommand{*this, m_jobs, m_files, index, batches, std::move(request)};
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
            makeImport(SKY_SOURCE).apply(scene);
        }
        return {};
    }

    void publishEdits(const TechEngine::Scene&) override {
        if (!index.getRecords().empty()) {
            registered.store(true);
        }
    }
};

TEST_CASE("the image import command returns before the job is done and writes the texture on a worker", "[editor][edit][import][image]") {
    ImportSkyAppProbe app{"importImageWritesFile"};
    app.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    ImportSkyCommand command = app.makeImport(SKY_SOURCE);

    command.apply(AppTestAccess::scene(app));

    REQUIRE(app.batches.size() == 1);
    CHECK(app.batches[0].valid());
    app.waitForImports();

    std::vector<std::byte> bytes;
    REQUIRE(app.files().read(SKY_TEXTURE, bytes) == std::error_code{});
    TechEngine::Texture texture;
    REQUIRE(texture.read(bytes) == std::error_code{});
    CHECK(texture.id.valid());
    CHECK(texture.format == TechEngine::TextureFormat::Rgb9E5);
    CHECK(texture.mips.size() == 4);

    SECTION("with no simulation running, the index entry is refused and the index stays as it was") {
        CHECK(app.index.getRecords().empty());
    }

    SECTION("a second import of the same source is refused by its job and changes no file") {
        app.makeImport(SKY_SOURCE).apply(AppTestAccess::scene(app));
        app.waitForImports();

        std::vector<std::byte> after;
        REQUIRE(app.files().read(SKY_TEXTURE, after) == std::error_code{});
        CHECK(after == bytes);
    }
}

TEST_CASE("an image import whose job fails writes nothing and registers nothing", "[editor][edit][import][image]") {
    ImportSkyAppProbe app{"importImageFails"};
    app.scratch.writeFile("assets/client/Sky.png", "not read");

    app.makeImport("project://assets/client/Sky.png").apply(AppTestAccess::scene(app));
    app.makeImport("project://assets/client/Missing.hdr").apply(AppTestAccess::scene(app));
    app.waitForImports();

    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Sky.tetexture"));
    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Missing.tetexture"));
    CHECK(app.index.getRecords().empty());
}

TEST_CASE("a running simulation adds the imported texture to the index", "[editor][edit][import][image][integration]") {
    ImportSkyAppProbe app{"importImageRegisters"};
    app.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
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
    REQUIRE(records.size() == 1);

    std::vector<std::byte> bytes;
    REQUIRE(app.files().read(SKY_TEXTURE, bytes) == std::error_code{});
    TechEngine::Texture texture;
    REQUIRE(texture.read(bytes) == std::error_code{});
    AssetRecord found;
    REQUIRE(app.index.find(texture.id, found));
    CHECK(found.typeTag == TechEngine::TEXTURE_TYPE_TAG);
    CHECK(found.side == TechEngine::Side::Client);
    CHECK(found.path == SKY_TEXTURE);
}
