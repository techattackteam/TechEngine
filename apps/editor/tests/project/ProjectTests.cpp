#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/assets/PackSource.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>
#include <project/Project.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::AssetIndex;
using TechEngine::BootRecord;
using TechEngine::ComponentRegistry;
using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::FileStatus;
using TechEngine::MESH_TYPE_TAG;
using TechEngine::MountTable;
using TechEngine::PackEntry;
using TechEngine::PackReader;
using TechEngine::PackSource;
using TechEngine::Project;
using TechEngine::ProjectError;
using TechEngine::ResourceId;
using TechEngine::Role;
using TechEngine::SCENE_KIND;
using TechEngine::SCENE_TYPE_TAG;
using TechEngine::Side;
using TechEngineTests::assetBytes;
using TechEngineTests::ScratchDirectory;
using TechEngineTests::writeBytes;

namespace {
    // mounts is declared before files, so the reference files captures is already alive; the
    // mount() call lands afterwards because FileAccess only stores a pointer to the table.
    struct MountedProject {
        ScratchDirectory scratch;
        MountTable mounts;
        FileAccess files;

        explicit MountedProject(std::string_view name) : scratch{name}, files{mounts} {
            mounts.mount("project", scratch.root());
        }
    };

    constexpr std::string_view GOOD_MANIFEST = "name = \"Sandbox\"\n";

    constexpr ResourceId EXPORT_SCENE_ID{0x1100000000000001};
    constexpr ResourceId EXPORT_CLIENT_ID{0x2200000000000002};
    constexpr ResourceId EXPORT_SERVER_ID{0x3300000000000003};
    constexpr ResourceId EXPORT_UNINDEXED_ID{0x4400000000000004};
    constexpr ResourceId EXPORT_MESH_ID{0x5500000000000005};
    constexpr ResourceId EXPORT_SERVER_MESH_ID{0x6600000000000006};
    constexpr ResourceId EXPORT_LINKED_SCENE_ID{0x7700000000000007};

    void registerExportComponents(ComponentRegistry& registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        registry.registerComponent<TechEngine::MeshRenderer>(TechEngine::MeshRenderer::tag);
    }

    std::vector<std::byte> sceneReferencing(const ResourceId id, const std::vector<ResourceId>& meshes) {
        ComponentRegistry registry;
        registerExportComponents(registry);
        TechEngine::Scene scene(registry);
        for (const ResourceId mesh: meshes) {
            const TechEngine::Entity entity = scene.createEntity();
            scene.addComponent<TechEngine::MeshRenderer>(entity, TechEngine::MeshRenderer{TechEngine::AssetRef<TechEngine::Mesh>{mesh, {}}});
        }
        std::vector<std::byte> bytes;
        TechEngine::SceneAsset{id}.write(scene, bytes);
        return bytes;
    }

    class ExportScratch {
    public:
        explicit ExportScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts), m_outputFiles(m_outputMounts) {
            m_scratch.makeDirectory("assets/common");
            m_scratch.makeDirectory("assets/client");
            m_scratch.makeDirectory("assets/server");
            m_mounts.mount("project", m_scratch.root());
            m_outputMounts.mount("out", outFolder() / "Export" / "assets");
            registerExportComponents(m_registry);

            m_sceneBytes = sceneReferencing(EXPORT_SCENE_ID, {});
            writeBytes(m_scratch, "assets/common/start.tescene", m_sceneBytes);
            writeBytes(m_scratch, "assets/client/menu.tescene", sceneReferencing(EXPORT_CLIENT_ID, {}));
            writeBytes(m_scratch, "assets/server/arena.tescene", sceneReferencing(EXPORT_SERVER_ID, {}));
        }

        void writeAsset(const std::string_view relative, const std::vector<std::byte>& bytes) const {
            writeBytes(m_scratch, std::filesystem::path(relative), bytes);
        }

        void open(const std::string_view manifest) {
            m_scratch.writeFile("project.toml", manifest);
            REQUIRE(m_project.load(m_files, "project://project.toml") == std::error_code{});
            REQUIRE(m_index.build(m_files, "project://assets") == std::error_code{});
        }

        std::error_code runExport() {
            return m_project.exportTo(Role::Client, m_index, m_files, m_registry, outFolder());
        }

        std::filesystem::path outFolder() const {
            return m_scratch.root() / "export";
        }

        const FileAccess& outputFiles() const {
            return m_outputFiles;
        }

        const std::vector<std::byte>& sceneBytes() const {
            return m_sceneBytes;
        }

        BootRecord readBootRecord() const {
            PackReader common{m_outputFiles};
            REQUIRE(common.open("out://common.tepack") == std::error_code{});
            PackEntry entry;
            REQUIRE(common.bootRecord(entry));
            std::vector<std::byte> bytes;
            REQUIRE(common.read(entry, bytes) == std::error_code{});
            BootRecord record;
            REQUIRE(record.read(bytes) == std::error_code{});
            return record;
        }

    private:
        ScratchDirectory m_scratch;
        MountTable m_mounts;
        FileAccess m_files;
        MountTable m_outputMounts;
        FileAccess m_outputFiles;
        Project m_project;
        AssetIndex m_index;
        ComponentRegistry m_registry;
        std::vector<std::byte> m_sceneBytes;
    };
}

static std::string manifestNaming(const ResourceId scene) {
    return std::format("name = \"Export\"\n\n[export.client]\nstartupScene = \"{:#018x}\"\n", scene.value());
}

static std::size_t entryCount(const FileAccess& files, const std::string_view pack) {
    PackReader reader{files};
    REQUIRE(reader.open(pack) == std::error_code{});
    return reader.getEntries().size();
}

TEST_CASE("load reads the name and derives the root", "[editor][project]") {
    MountedProject env{"projectLoadGood"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project project;

    REQUIRE(project.load(env.files, "project://project.toml") == std::error_code{});
    CHECK(project.name() == "Sandbox");
    CHECK(project.root() == env.scratch.root());
}

TEST_CASE("load returns ParseFailed on a malformed manifest", "[editor][project]") {
    MountedProject env{"projectLoadMalformed"};
    env.scratch.writeFile("project.toml", "name = [unclosed\n");

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == ProjectError::ParseFailed);
}

TEST_CASE("load passes NotFound through when the manifest is missing", "[editor][project]") {
    MountedProject env{"projectLoadMissing"};

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == FileError::NotFound);
}

TEST_CASE("load passes InvalidPath through for a manifest path that escapes the root", "[editor][project]") {
    MountedProject env{"projectLoadEscapes"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project project;

    CHECK(project.load(env.files, "project://../project.toml") == FileError::InvalidPath);
}

TEST_CASE("load returns SchemaInvalid when name is missing", "[editor][project]") {
    MountedProject env{"projectLoadNoName"};
    env.scratch.writeFile("project.toml", "unrelated = 1\n");

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == ProjectError::SchemaInvalid);
}

TEST_CASE("load returns SchemaInvalid when name is not a string", "[editor][project]") {
    MountedProject env{"projectLoadBadName"};
    env.scratch.writeFile("project.toml", "name = 7\n");

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == ProjectError::SchemaInvalid);
}

TEST_CASE("save round-trips through load", "[editor][project]") {
    MountedProject env{"projectSaveRoundTrip"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project source;
    REQUIRE(source.load(env.files, "project://project.toml") == std::error_code{});
    REQUIRE(source.save(env.files, "project://copy.toml") == std::error_code{});

    Project reloaded;
    REQUIRE(reloaded.load(env.files, "project://copy.toml") == std::error_code{});
    CHECK(reloaded.name() == source.name());
}

TEST_CASE("load reads an optional client startup scene", "[editor][project]") {
    MountedProject env{"projectLoadClientStartupScene"};
    env.scratch.writeFile("project.toml", "name = \"Sandbox\"\n\n[export.client]\nstartupScene = \"0x3f9a2c41d07e88b5\"\n");

    Project project;

    REQUIRE(project.load(env.files, "project://project.toml") == std::error_code{});
    CHECK(project.getExportStartupScene(Role::Client) == ResourceId(0x3f9a2c41d07e88b5));
}

TEST_CASE("load leaves the client startup scene null when the key is absent", "[editor][project]") {
    MountedProject env{"projectLoadNoClientStartupScene"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project project;

    REQUIRE(project.load(env.files, "project://project.toml") == std::error_code{});
    CHECK_FALSE(project.getExportStartupScene(Role::Client).valid());
}

TEST_CASE("load returns SchemaInvalid when the client startup scene is not a string", "[editor][project]") {
    MountedProject env{"projectLoadClientStartupSceneInteger"};
    env.scratch.writeFile("project.toml", "name = \"Sandbox\"\n\n[export.client]\nstartupScene = 42\n");

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == ProjectError::SchemaInvalid);
}

TEST_CASE("load returns SchemaInvalid when the client startup scene is not a hex id", "[editor][project]") {
    MountedProject env{"projectLoadClientStartupSceneText"};
    env.scratch.writeFile("project.toml", "name = \"Sandbox\"\n\n[export.client]\nstartupScene = \"level one\"\n");

    Project project;

    CHECK(project.load(env.files, "project://project.toml") == ProjectError::SchemaInvalid);
}

TEST_CASE("a failed load keeps the previous client startup scene and name", "[editor][project]") {
    MountedProject env{"projectLoadClientStartupSceneKept"};
    env.scratch.writeFile("project.toml", "name = \"Sandbox\"\n\n[export.client]\nstartupScene = \"0x00000000000000aa\"\n");
    env.scratch.writeFile("broken.toml", "name = \"Other\"\n\n[export.client]\nstartupScene = \"level one\"\n");

    Project project;
    REQUIRE(project.load(env.files, "project://project.toml") == std::error_code{});

    CHECK(project.load(env.files, "project://broken.toml") == ProjectError::SchemaInvalid);
    CHECK(project.name() == "Sandbox");
    CHECK(project.getExportStartupScene(Role::Client) == ResourceId(0xaa));
}

TEST_CASE("save round-trips the client startup scene through load", "[editor][project]") {
    MountedProject env{"projectSaveClientStartupScene"};
    env.scratch.writeFile("project.toml", GOOD_MANIFEST);

    Project source;
    REQUIRE(source.load(env.files, "project://project.toml") == std::error_code{});

    SECTION("a set id, including one with the top bit set") {
        source.setExportStartupScene(Role::Client, ResourceId(0xf00dcafe12345678));
        REQUIRE(source.save(env.files, "project://copy.toml") == std::error_code{});

        Project reloaded;
        REQUIRE(reloaded.load(env.files, "project://copy.toml") == std::error_code{});
        CHECK(reloaded.getExportStartupScene(Role::Client) == ResourceId(0xf00dcafe12345678));
        CHECK(reloaded.name() == "Sandbox");
    }

    SECTION("a null id") {
        REQUIRE(source.save(env.files, "project://copy.toml") == std::error_code{});

        Project reloaded;
        REQUIRE(reloaded.load(env.files, "project://copy.toml") == std::error_code{});
        CHECK_FALSE(reloaded.getExportStartupScene(Role::Client).valid());
    }
}

TEST_CASE("exportTo writes only the client target's packs under <folder>/<project name>/assets", "[editor][project][export]") {
    ExportScratch scratch{"projectExportWritesPacks"};
    scratch.open(manifestNaming(EXPORT_SCENE_ID));

    REQUIRE(scratch.runExport() == std::error_code{});

    FileStatus status;
    CHECK(scratch.outputFiles().status("out://common.tepack", status) == std::error_code{});
    CHECK(scratch.outputFiles().status("out://client.tepack", status) == std::error_code{});
    CHECK(scratch.outputFiles().status("out://server.tepack", status) == FileError::NotFound);
}

TEST_CASE("exportTo packs only what the startup scene reaches, into common", "[editor][project][export]") {
    ExportScratch scratch{"projectExportPacksReached"};
    scratch.open(manifestNaming(EXPORT_SCENE_ID));
    REQUIRE(scratch.runExport() == std::error_code{});

    PackReader common{scratch.outputFiles()};
    REQUIRE(common.open("out://common.tepack") == std::error_code{});
    REQUIRE(common.getEntries().size() == 1);
    PackEntry entry;
    REQUIRE(common.find(EXPORT_SCENE_ID, entry));
    CHECK(entry.typeTag == SCENE_TYPE_TAG);
    std::vector<std::byte> payload;
    REQUIRE(common.read(entry, payload) == std::error_code{});
    CHECK(payload == scratch.sceneBytes());

    CHECK(entryCount(scratch.outputFiles(), "out://client.tepack") == 0);
}

TEST_CASE("exportTo puts a boot record for the client role in common", "[editor][project][export]") {
    ExportScratch scratch{"projectExportBootRecord"};
    scratch.open(manifestNaming(EXPORT_SCENE_ID));
    REQUIRE(scratch.runExport() == std::error_code{});

    const BootRecord record = scratch.readBootRecord();

    CHECK(record.startupScene == EXPORT_SCENE_ID);
    CHECK(record.systemSettings.empty());
    REQUIRE(record.rolePacks.size() == 1);
    CHECK(record.rolePacks[0].role == Role::Client);
    CHECK(std::ranges::is_permutation(record.rolePacks[0].packs, std::vector<std::string>{"common.tepack", "client.tepack"}));
    REQUIRE(record.sides.size() == 1);
    CHECK(record.sides[0].id == EXPORT_SCENE_ID);
    CHECK(record.sides[0].side == Side::Common);
}

TEST_CASE("the client role reads the startup scene from the exported packs", "[editor][project][export]") {
    ExportScratch scratch{"projectExportClientReads"};
    scratch.open(manifestNaming(EXPORT_SCENE_ID));
    REQUIRE(scratch.runExport() == std::error_code{});
    const BootRecord record = scratch.readBootRecord();

    PackSource source{scratch.outputFiles()};
    REQUIRE(source.open(record, Role::Client, "out://") == std::error_code{});
    REQUIRE(source.find(EXPORT_SCENE_ID) == AssetAvailability::Present);
    std::vector<std::byte> bytes;
    REQUIRE(source.read(EXPORT_SCENE_ID, bytes) == std::error_code{});
    CHECK(bytes == scratch.sceneBytes());
}

TEST_CASE("exportTo packs a client startup scene into client.tepack", "[editor][project][export]") {
    ExportScratch scratch{"projectExportClientScene"};
    scratch.open(manifestNaming(EXPORT_CLIENT_ID));
    REQUIRE(scratch.runExport() == std::error_code{});
    const BootRecord record = scratch.readBootRecord();

    CHECK(record.startupScene == EXPORT_CLIENT_ID);
    REQUIRE(record.sides.size() == 1);
    CHECK(record.sides[0].id == EXPORT_CLIENT_ID);
    CHECK(record.sides[0].side == Side::Client);
    CHECK(entryCount(scratch.outputFiles(), "out://common.tepack") == 0);
    CHECK(entryCount(scratch.outputFiles(), "out://client.tepack") == 1);

    PackSource source{scratch.outputFiles()};
    REQUIRE(source.open(record, Role::Client, "out://") == std::error_code{});
    CHECK(source.find(EXPORT_CLIENT_ID) == AssetAvailability::Present);
}

TEST_CASE("exportTo returns NoStartupScene when the target names none", "[editor][project][export]") {
    ExportScratch scratch{"projectExportNoScene"};
    scratch.open(GOOD_MANIFEST);

    CHECK(scratch.runExport() == ProjectError::NoStartupScene);
}

TEST_CASE("exportTo returns UnknownStartupScene when the startup scene is not indexed", "[editor][project][export]") {
    ExportScratch scratch{"projectExportUnknownScene"};
    scratch.open(manifestNaming(EXPORT_UNINDEXED_ID));

    CHECK(scratch.runExport() == ProjectError::UnknownStartupScene);
}

TEST_CASE("exportTo returns StartupSceneNotInTarget for a client target whose scene is a server asset", "[editor][project][export]") {
    ExportScratch scratch{"projectExportServerScene"};
    scratch.open(manifestNaming(EXPORT_SERVER_ID));

    CHECK(scratch.runExport() == ProjectError::StartupSceneNotInTarget);
}

TEST_CASE("exportTo numbers the project folder when that name is taken", "[editor][project][export]") {
    ExportScratch scratch{"projectExportNumbersFolder"};
    scratch.open(manifestNaming(EXPORT_SCENE_ID));
    const std::filesystem::path out = scratch.outFolder();
    std::filesystem::create_directories(out / "Export");
    std::filesystem::create_directories(out / "Export(1)");
    {
        std::ofstream keep{out / "Export" / "keep.txt"};
        keep << "keep";
    }

    REQUIRE(scratch.runExport() == std::error_code{});
    REQUIRE(scratch.runExport() == std::error_code{});

    CHECK(std::filesystem::exists(out / "Export(2)" / "assets" / "common.tepack"));
    CHECK(std::filesystem::exists(out / "Export(3)" / "assets" / "common.tepack"));
    CHECK_FALSE(std::filesystem::exists(out / "Export" / "assets"));
    CHECK_FALSE(std::filesystem::exists(out / "Export(1)" / "assets"));
    CHECK(std::filesystem::exists(out / "Export" / "keep.txt"));
}

TEST_CASE("a failed export claims no project folder", "[editor][project][export]") {
    ExportScratch scratch{"projectExportFailsCleanly"};
    scratch.open(GOOD_MANIFEST);

    REQUIRE(scratch.runExport() == ProjectError::NoStartupScene);

    CHECK_FALSE(std::filesystem::exists(scratch.outFolder()));
}

static std::vector<std::byte> meshBytes(const ResourceId id) {
    const TechEngine::Mesh mesh{id, {TechEngine::Vec3(-0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.0f, 0.5f, 0.0f)}, {0, 1, 2}};
    std::vector<std::byte> bytes;
    mesh.write(bytes);
    return bytes;
}

TEST_CASE("exportTo packs the mesh the startup scene references into client.tepack", "[editor][project][export]") {
    ExportScratch scratch{"projectExportPacksMesh"};
    const std::vector<std::byte> mesh = meshBytes(EXPORT_MESH_ID);
    scratch.writeAsset("assets/client/Triangle.temesh", mesh);
    scratch.writeAsset("assets/common/linked.tescene", sceneReferencing(EXPORT_LINKED_SCENE_ID, {EXPORT_MESH_ID, EXPORT_MESH_ID}));
    scratch.open(manifestNaming(EXPORT_LINKED_SCENE_ID));

    REQUIRE(scratch.runExport() == std::error_code{});

    CHECK(entryCount(scratch.outputFiles(), "out://common.tepack") == 1);
    PackReader client{scratch.outputFiles()};
    REQUIRE(client.open("out://client.tepack") == std::error_code{});
    REQUIRE(client.getEntries().size() == 1);
    PackEntry entry;
    REQUIRE(client.find(EXPORT_MESH_ID, entry));
    CHECK(entry.typeTag == MESH_TYPE_TAG);
    std::vector<std::byte> payload;
    REQUIRE(client.read(entry, payload) == std::error_code{});
    CHECK(payload == mesh);

    const BootRecord record = scratch.readBootRecord();
    REQUIRE(record.sides.size() == 2);
    const auto meshSide = std::ranges::find(record.sides, EXPORT_MESH_ID, &TechEngine::ResourceSide::id);
    REQUIRE(meshSide != record.sides.end());
    CHECK(meshSide->side == Side::Client);

    PackSource source{scratch.outputFiles()};
    REQUIRE(source.open(record, Role::Client, "out://") == std::error_code{});
    CHECK(source.find(EXPORT_MESH_ID) == AssetAvailability::Present);
}

TEST_CASE("exportTo lists a referenced server asset as absent for the client without packing it", "[editor][project][export]") {
    ExportScratch scratch{"projectExportServerReference"};
    scratch.writeAsset("assets/server/Collider.temesh", meshBytes(EXPORT_SERVER_MESH_ID));
    scratch.writeAsset("assets/common/linked.tescene", sceneReferencing(EXPORT_LINKED_SCENE_ID, {EXPORT_SERVER_MESH_ID}));
    scratch.open(manifestNaming(EXPORT_LINKED_SCENE_ID));

    REQUIRE(scratch.runExport() == std::error_code{});

    CHECK(entryCount(scratch.outputFiles(), "out://common.tepack") == 1);
    CHECK(entryCount(scratch.outputFiles(), "out://client.tepack") == 0);
    const BootRecord record = scratch.readBootRecord();
    PackSource source{scratch.outputFiles()};
    REQUIRE(source.open(record, Role::Client, "out://") == std::error_code{});
    CHECK(source.find(EXPORT_SERVER_MESH_ID) == AssetAvailability::AbsentForRole);
}

TEST_CASE("exportTo returns UnknownReference when the startup scene references an unindexed asset, and claims no folder", "[editor][project][export]") {
    ExportScratch scratch{"projectExportUnknownReference"};
    scratch.writeAsset("assets/common/linked.tescene", sceneReferencing(EXPORT_LINKED_SCENE_ID, {EXPORT_UNINDEXED_ID}));
    scratch.open(manifestNaming(EXPORT_LINKED_SCENE_ID));

    REQUIRE(scratch.runExport() == ProjectError::UnknownReference);

    CHECK_FALSE(std::filesystem::exists(scratch.outFolder()));
}

TEST_CASE("exportTo returns the read error of a startup scene that does not parse, and claims no folder", "[editor][project][export]") {
    ExportScratch scratch{"projectExportSceneDoesNotParse"};
    scratch.writeAsset("assets/common/broken.tescene", assetBytes(SCENE_KIND, EXPORT_LINKED_SCENE_ID, "not a scene"));
    scratch.open(manifestNaming(EXPORT_LINKED_SCENE_ID));

    CHECK(scratch.runExport() == TechEngine::ReadError::Truncated);

    CHECK_FALSE(std::filesystem::exists(scratch.outFolder()));
}
