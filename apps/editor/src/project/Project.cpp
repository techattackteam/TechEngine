#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/PackWriter.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

#include <assets/AssetIndex.hpp>
#include <project/Project.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    struct ExportTargetSides {
        Role role{};
        bool shipsClient = false;
        bool shipsServer = false;
    };

    struct ExportPackFile {
        Side side = Side::Common;
        const char* name = nullptr;
    };

    struct ExportPack {
        ExportPackFile file;
        PackWriter writer;
    };

    static constexpr std::array<ExportTargetSides, 1> EXPORT_TARGETS{{
        {Role::Client, true, false},
        // TODO: add the server targets when a server executable or hosting exists.
        // {Role::DedicatedServer, false, true},
        // {Role::ListenServer, true, true},
    }};

    static constexpr std::array<ExportPackFile, 3> PACK_FILES{{
        {Side::Common, "common.tepack"},
        {Side::Client, "client.tepack"},
        {Side::Server, "server.tepack"},
    }};

    std::error_code Project::load(const FileAccess& files, std::string_view manifestPath) {
        std::vector<std::byte> bytes;
        const std::error_code readError = files.read(manifestPath, bytes);
        if (readError) {
            return readError;
        }

        const std::string_view text = bytes.empty() ? std::string_view{} : std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()};

        const toml::parse_result parsed = toml::parse(text);
        if (!parsed) {
            return ProjectError::ParseFailed;
        }

        const std::optional<std::string> name = parsed["name"].value<std::string>();
        if (!name) {
            return ProjectError::SchemaInvalid;
        }

        std::filesystem::path physical;
        const std::error_code resolveError = files.resolve(manifestPath, physical);
        if (resolveError) {
            return resolveError;
        }

        const auto sceneNode = parsed["export"]["client"]["startupScene"];
        ResourceId clientStartupScene;
        if (sceneNode) {
            const std::optional<std::string> sceneID = sceneNode.value_exact<std::string>();
            if (!sceneID || sceneID->size() != 18 || !sceneID->starts_with("0x")) {
                return ProjectError::SchemaInvalid;
            }

            std::uint64_t value = 0;
            const char* const digitsEnd = sceneID->data() + sceneID->size();
            const std::from_chars_result parsedId = std::from_chars(sceneID->data() + 2, digitsEnd, value, 16);
            if (parsedId.ec != std::errc{} || parsedId.ptr != digitsEnd) {
                return ProjectError::SchemaInvalid;
            }
            clientStartupScene = ResourceId(value);
        }

        m_root = physical.parent_path();
        m_name = *name;
        m_clientStartupScene = clientStartupScene;
        return {};
    }

    std::error_code Project::save(FileAccess& files, const std::string_view manifestPath) const {
        toml::table manifest;
        manifest.insert_or_assign("name", m_name);
        if (m_clientStartupScene.valid()) {
            toml::table client;
            client.insert_or_assign("startupScene", std::format("{:#018x}", m_clientStartupScene.value()));
            toml::table exportTargets;
            exportTargets.insert_or_assign("client", std::move(client));
            manifest.insert_or_assign("export", std::move(exportTargets));
        }

        std::ostringstream stream;
        stream << manifest;
        const std::string text = stream.str();

        const std::span bytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};

        const std::error_code writeError = files.write(manifestPath, bytes);
        if (writeError) {
            return writeError;
        }
        return {};
    }

    std::error_code Project::exportTo(const Role target, const AssetIndex& index, const FileAccess& files, ComponentRegistry& registry, const std::filesystem::path& runtimePath, const std::filesystem::path& exportFolder) const {
        const auto exportTarget = std::ranges::find(EXPORT_TARGETS, target, &ExportTargetSides::role);
        TE_CHECK(exportTarget != EXPORT_TARGETS.end(), "Export target {0} is not built yet", static_cast<int>(target));
        const auto ships = [&exportTarget](const Side side) {
            return side == Side::Common || (side == Side::Client && exportTarget->shipsClient) || (side == Side::Server && exportTarget->shipsServer);
        };

        const ResourceId startupScene = getExportStartupScene(target);
        if (!startupScene.valid()) {
            return ProjectError::NoStartupScene;
        }

        AssetRecord scene;
        if (!index.find(startupScene, scene)) {
            return ProjectError::UnknownStartupScene;
        }
        if (!ships(scene.side)) {
            return ProjectError::StartupSceneNotInTarget;
        }

        std::vector<std::byte> scenePayload;
        const std::error_code readError = files.read(scene.path, scenePayload);
        if (readError) {
            return readError;
        }

        std::vector<AssetRecord> reached;
        const std::error_code reachError = collectReachedAssets(scene, scenePayload, index, registry, reached);
        if (reachError) {
            return reachError;
        }

        std::vector<std::vector<std::byte>> payloads(reached.size());
        payloads[0] = std::move(scenePayload);
        for (std::size_t i = 1; i < reached.size(); i++) {
            if (!ships(reached[i].side)) {
                continue;
            }
            const std::error_code assetReadError = files.read(reached[i].path, payloads[i]);
            if (assetReadError) {
                return assetReadError;
            }
        }

        std::vector<ExportPack> packs;
        RolePacks rolePacks;
        rolePacks.role = target;
        for (const ExportPackFile& file: PACK_FILES) {
            if (ships(file.side)) {
                packs.emplace_back().file = file;
                rolePacks.packs.emplace_back(file.name);
            }
        }

        BootRecord bootRecord;
        bootRecord.startupScene = scene.id;
        bootRecord.rolePacks = {rolePacks};
        for (const AssetRecord& record: reached) {
            bootRecord.sides.push_back({record.id, record.side});
        }
        std::vector<std::byte> bootBytes;
        bootRecord.write(bootBytes);

        for (ExportPack& pack: packs) {
            for (std::size_t i = 0; i < reached.size(); i++) {
                if (pack.file.side == reached[i].side) {
                    pack.writer.add({reached[i].id, reached[i].typeTag, reached[i].path, payloads[i]});
                }
            }
            if (pack.file.side == Side::Common) {
                pack.writer.setBootRecord(bootBytes);
            }
        }

        MountTable outputMounts;
        outputMounts.mount("runtime", runtimePath.parent_path());
        outputMounts.mount("export", exportFolder);
        FileAccess output{outputMounts};

        const std::string runtimeName = runtimePath.filename().string();
        FileStatus runtimeStatus;
        const std::error_code runtimeError = output.status("runtime://" + runtimeName, runtimeStatus);
        if (runtimeError) {
            TE_LOGGER_ERROR("The runtime executable {0} cannot be copied into the export: {1}", runtimePath.string(), runtimeError.message());
            return runtimeError;
        }
        if (runtimeStatus.isDirectory) {
            return FileError::IsADirectory;
        }

        const std::string projectFolder = "export://" + m_name;
        const std::error_code folderError = output.createDirectory(projectFolder);
        if (folderError && folderError != FileError::AlreadyExists) {
            return folderError;
        }

        // Windows locks a running executable, so a runtime still running fails here, before any pack changes.
        const std::error_code replaceError = replaceRuntime(output, projectFolder, runtimeName);
        if (replaceError) {
            return replaceError;
        }

        const std::string assetsFolder = projectFolder + "/assets";
        const std::error_code assetsError = output.createDirectory(assetsFolder);
        if (assetsError && assetsError != FileError::AlreadyExists) {
            return assetsError;
        }

        for (const ExportPack& pack: packs) {
            std::vector<std::byte> bytes;
            pack.writer.write(bytes);
            const std::error_code writeError = output.write(assetsFolder + "/" + pack.file.name, bytes);
            if (writeError) {
                return writeError;
            }
        }
        for (const ExportPackFile& file: PACK_FILES) {
            if (ships(file.side)) {
                continue;
            }
            const std::error_code removeError = output.remove(assetsFolder + "/" + file.name, false);
            if (removeError && removeError != FileError::NotFound) {
                return removeError;
            }
        }
        TE_LOGGER_INFO("Exported the project to {0}", (exportFolder / m_name).string());
        return {};
    }

    const std::filesystem::path& Project::root() const {
        return m_root;
    }

    const std::string& Project::name() const {
        return m_name;
    }

    // TODO: keep a startup scene per export target, and load and save each `[export.<target>]`
    // table, when the server targets land.
    ResourceId Project::getExportStartupScene(const Role target) const {
        return target == Role::Client ? m_clientStartupScene : ResourceId{};
    }

    void Project::setExportStartupScene(const Role target, const ResourceId id) {
        TE_CHECK(target == Role::Client, "Only the client export target exists so far");
        m_clientStartupScene = id;
    }

    std::error_code Project::collectReachedAssets(const AssetRecord& scene, const std::span<const std::byte> sceneBytes, const AssetIndex& index, ComponentRegistry& registry, std::vector<AssetRecord>& out) const {
        Scene parsed(registry);
        SceneAsset asset;
        const std::error_code parseError = asset.read(sceneBytes, parsed);
        if (parseError) {
            return parseError;
        }
        std::vector<ResourceId> references;
        asset.collectReferences(parsed, references);

        std::vector<AssetRecord> reached = {scene};
        for (const ResourceId reference: references) {
            // PackWriter::add rejects an id added twice. The runtime's load fails this scene as cyclic.
            if (reference == scene.id) {
                continue;
            }
            AssetRecord record;
            if (!index.find(reference, record)) {
                TE_LOGGER_ERROR("The startup scene {0:#018x} references {1:#018x}, which is not an indexed asset", scene.id.value(), reference.value());
                return ProjectError::UnknownReference;
            }
            reached.push_back(std::move(record));
        }
        out = std::move(reached);
        return {};
    }

    std::error_code Project::replaceRuntime(FileAccess& output, const std::string& projectFolder, const std::string& runtimeName) const {
        const std::string destination = projectFolder + "/" + runtimeName;
        const std::error_code removeError = output.remove(destination, false);
        if (removeError && removeError != FileError::NotFound) {
            TE_LOGGER_ERROR("Failed to replace the runtime in {0}: {1}", destination, removeError.message());
            return removeError;
        }
        return output.copy("runtime://" + runtimeName, destination);
    }
}
