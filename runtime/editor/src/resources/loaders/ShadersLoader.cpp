#include "ShadersLoader.hpp"

#include "core/Logger.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <unordered_set>

namespace TechEngine {
    namespace {
        struct StageSuffix {
            std::string_view suffix;
            ShaderType type;
        };

        // Longer variants are listed first so, for example, "vertex" is matched before "vert".
        constexpr std::array<StageSuffix, 12> kStageSuffixes = {{
            {"vertex", ShaderType::Vertex},
            {"vert", ShaderType::Vertex},
            {"fragment", ShaderType::Fragment},
            {"frag", ShaderType::Fragment},
            {"geometry", ShaderType::Geometry},
            {"geom", ShaderType::Geometry},
            {"compute", ShaderType::Compute},
            {"comp", ShaderType::Compute},
            {"tesscontrol", ShaderType::TessellationControl},
            {"tesc", ShaderType::TessellationControl},
            {"tesseval", ShaderType::TessellationEvaluation},
            {"tese", ShaderType::TessellationEvaluation},
        }};

        std::string toLower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        const StageSuffix* matchStageSuffix(const std::string& loweredStem) {
            for (const StageSuffix& stage: kStageSuffixes) {
                if (loweredStem.size() >= stage.suffix.size() &&
                    loweredStem.compare(loweredStem.size() - stage.suffix.size(), stage.suffix.size(), stage.suffix) == 0) {
                    return &stage;
                }
            }
            return nullptr;
        }

        std::string shaderTypeToString(ShaderType type) {
            switch (type) {
                case ShaderType::Vertex: return "Vertex";
                case ShaderType::Fragment: return "Fragment";
                case ShaderType::Geometry: return "Geometry";
                case ShaderType::Compute: return "Compute";
                case ShaderType::TessellationControl: return "TessellationControl";
                case ShaderType::TessellationEvaluation: return "TessellationEvaluation";
            }
            return "Vertex";
        }

        bool shaderTypeFromString(const std::string& value, ShaderType& outType) {
            const std::string lowered = toLower(value);
            if (lowered == "vertex" || lowered == "vert") { outType = ShaderType::Vertex; return true; }
            if (lowered == "fragment" || lowered == "frag") { outType = ShaderType::Fragment; return true; }
            if (lowered == "geometry" || lowered == "geom") { outType = ShaderType::Geometry; return true; }
            if (lowered == "compute" || lowered == "comp") { outType = ShaderType::Compute; return true; }
            if (lowered == "tesscontrol" || lowered == "tesc") { outType = ShaderType::TessellationControl; return true; }
            if (lowered == "tesseval" || lowered == "tese") { outType = ShaderType::TessellationEvaluation; return true; }
            return false;
        }
    }

    ShadersLoader::ShadersLoader(ResourceSystem& resourcesSystem,
                                 FileSystem& fileSystem,
                                 IFileWatcher& fileWatcher) : m_resourcesSystem(resourcesSystem),
                                                              m_fileSystem(fileSystem),
                                                              m_fileWatcher(fileWatcher) {
        m_subscriptionId = m_fileWatcher.subscribe([this](const FileChangedEvent& event) {
            onFileChanged(event);
        });
    }

    ShadersLoader::~ShadersLoader() {
        m_fileWatcher.unsubscribe(m_subscriptionId);
    }

    std::vector<std::filesystem::path> ShadersLoader::generateMissingShaderManifests(
        const std::vector<std::filesystem::path>& sourceFiles,
        const std::vector<std::filesystem::path>& existingManifests) const {
        // Every source file already referenced by a manifest is left alone; only the leftovers are
        // grouped into new programs so we never author a duplicate for an existing manifest.
        std::unordered_set<std::string> referencedSources;
        for (const std::filesystem::path& manifestPath: existingManifests) {
            ShaderManifest manifest;
            if (!readShaderManifest(manifestPath, manifest)) {
                continue;
            }
            const std::filesystem::path parent = m_fileSystem.getParentPath(manifestPath);
            for (const auto& [type, file]: manifest.stages) {
                referencedSources.insert(m_fileSystem.resolve(parent / file).string());
            }
        }

        struct PendingProgram {
            std::filesystem::path directory;
            std::string name;
            std::map<ShaderType, std::filesystem::path> stages;
        };
        std::map<std::string, PendingProgram> pending;

        for (const std::filesystem::path& source: sourceFiles) {
            if (referencedSources.count(m_fileSystem.resolve(source).string()) > 0) {
                continue;
            }
            const std::filesystem::path parent = m_fileSystem.getParentPath(source);
            const std::string name = shaderNameFromPath(source);
            const std::string key = m_fileSystem.resolve(parent).string() + "::" + name;

            PendingProgram& program = pending[key];
            program.directory = parent;
            program.name = name;
            program.stages[shaderTypeFromPath(source)] = source.filename();
        }

        std::vector<std::filesystem::path> created;
        for (const auto& [key, program]: pending) {
            const std::filesystem::path manifestPath = program.directory / (program.name + resourceExtension());
            if (m_fileSystem.status(manifestPath).exists) {
                continue;
            }
            if (writeShaderManifest(manifestPath, program.name, program.stages)) {
                created.push_back(manifestPath);
            }
        }
        return created;
    }

    std::shared_ptr<ShaderResource> ShadersLoader::loadShaderResource(const std::filesystem::path& manifestPath) const {
        ShaderManifest manifest;
        if (!readShaderManifest(manifestPath, manifest)) {
            return nullptr;
        }
        if (manifest.name.empty()) {
            TE_LOGGER_ERROR("[ShadersLoader] Shader manifest '{0}' has no name", manifestPath.string());
            return nullptr;
        }

        const std::filesystem::path parent = m_fileSystem.getParentPath(manifestPath);
        std::shared_ptr<ShaderResource> shaderResource = std::make_shared<ShaderResource>(manifest.name);
        for (const auto& [type, file]: manifest.stages) {
            const std::filesystem::path sourcePath = parent / file;
            Buffer buffer;
            if (!m_fileSystem.read(sourcePath, buffer) || !buffer) {
                TE_LOGGER_ERROR("[ShadersLoader] Failed to read shader source: {0}", sourcePath.string());
                continue;
            }
            shaderResource->setSource(type, std::string(static_cast<const char*>(buffer.data), buffer.size));
            buffer.release();
        }

        if (shaderResource->getSources().empty()) {
            TE_LOGGER_WARN("[ShadersLoader] Shader manifest '{0}' produced no stages, skipping", manifestPath.string());
            return nullptr;
        }

        // Re-register so the GPU relinks whenever a manifest (or one of its sources) changes.
        if (m_resourcesSystem.isShaderRegistered(manifest.name)) {
            m_resourcesSystem.unregisterShaderResource(manifest.name);
        }
        m_resourcesSystem.registerShaderResource(shaderResource);
        m_manifestPrograms[manifestPath.generic_string()] = manifest.name;
        return shaderResource;
    }

    bool ShadersLoader::writeShaderManifest(const std::filesystem::path& manifestPath,
                                            const std::string& name,
                                            const std::map<ShaderType, std::filesystem::path>& stages) const {
        YAML::Node root;
        root["name"] = name;
        YAML::Node stagesNode(YAML::NodeType::Map);
        for (const auto& [type, file]: stages) {
            stagesNode[shaderTypeToString(type)] = file.filename().string();
        }
        root["stages"] = stagesNode;

        const std::string content = YAML::Dump(root);
        Buffer buffer = Buffer::copy(content.data(), content.size());
        const bool written = m_fileSystem.write(manifestPath, buffer);
        buffer.release();
        if (!written) {
            TE_LOGGER_ERROR("[ShadersLoader] Failed to write shader manifest: {0}", manifestPath.string());
        }
        return written;
    }

    std::shared_ptr<ShaderResource> ShadersLoader::createShaderResource(const std::filesystem::path& manifestPath,
                                                                        const std::string& name,
                                                                        const std::map<ShaderType, std::filesystem::path>& stages) const {
        if (!writeShaderManifest(manifestPath, name, stages)) {
            return nullptr;
        }
        return loadShaderResource(manifestPath);
    }

    void ShadersLoader::removeShaderResource(const std::filesystem::path& manifestPath) const {
        const auto it = m_manifestPrograms.find(manifestPath.generic_string());
        if (it != m_manifestPrograms.end()) {
            m_resourcesSystem.unregisterShaderResource(it->second);
            m_manifestPrograms.erase(it);
            return;
        }
        // The manifest was never loaded this session; fall back to its file stem as a best-effort name.
        const std::string name = manifestPath.stem().string();
        if (m_resourcesSystem.isShaderRegistered(name)) {
            m_resourcesSystem.unregisterShaderResource(name);
        }
    }

    std::vector<std::string> ShadersLoader::sourceExtensions() const {
        return {".glsl"};
    }

    std::string ShadersLoader::resourceExtension() const {
        return ".teshader";
    }

    void ShadersLoader::onFileChanged(const FileChangedEvent& event) const {
        const std::string extension = event.virtualPath.extension().string();
        const bool isManifest = extension == resourceExtension();
        const bool isSource = canLoad(extension);
        if (!isManifest && !isSource) {
            return;
        }

        switch (event.action) {
            case FileChangeAction::Added:
                if (isManifest) {
                    loadShaderResource(event.virtualPath);
                } else {
                    ensureManifestForSource(event.virtualPath);
                }
                break;
            case FileChangeAction::Modified:
                if (isManifest) {
                    loadShaderResource(event.virtualPath);
                } else {
                    onSourceChanged(event.virtualPath);
                }
                break;
            case FileChangeAction::Removed:
                if (isManifest) {
                    removeShaderResource(event.virtualPath);
                } else {
                    onSourceChanged(event.virtualPath);
                }
                break;
            case FileChangeAction::Renamed:
                if (!event.renamedFromVirtualPath.empty()) {
                    const std::string fromExtension = event.renamedFromVirtualPath.extension().string();
                    if (fromExtension == resourceExtension()) {
                        removeShaderResource(event.renamedFromVirtualPath);
                    } else if (canLoad(fromExtension)) {
                        onSourceChanged(event.renamedFromVirtualPath);
                    }
                }
                if (isManifest) {
                    loadShaderResource(event.virtualPath);
                } else if (isSource) {
                    ensureManifestForSource(event.virtualPath);
                }
                break;
        }
    }

    bool ShadersLoader::readShaderManifest(const std::filesystem::path& manifestPath, ShaderManifest& outManifest) const {
        Buffer buffer;
        if (!m_fileSystem.read(manifestPath, buffer) || !buffer) {
            TE_LOGGER_ERROR("[ShadersLoader] Failed to read shader manifest: {0}", manifestPath.string());
            return false;
        }
        const std::string content(static_cast<const char*>(buffer.data), buffer.size);
        buffer.release();

        try {
            const YAML::Node root = YAML::Load(content);
            if (root["name"]) {
                outManifest.name = root["name"].as<std::string>();
            }
            const YAML::Node stagesNode = root["stages"];
            if (stagesNode && stagesNode.IsMap()) {
                for (const auto& stage: stagesNode) {
                    ShaderType type;
                    if (shaderTypeFromString(stage.first.as<std::string>(), type)) {
                        outManifest.stages[type] = stage.second.as<std::string>();
                    } else {
                        TE_LOGGER_WARN("[ShadersLoader] Unknown shader stage '{0}' in manifest '{1}'",
                                       stage.first.as<std::string>(), manifestPath.string());
                    }
                }
            }
        } catch (const YAML::Exception& exception) {
            TE_LOGGER_ERROR("[ShadersLoader] Failed to parse shader manifest '{0}': {1}", manifestPath.string(), exception.what());
            return false;
        }
        return true;
    }

    void ShadersLoader::onSourceChanged(const std::filesystem::path& sourcePath) const {
        const std::filesystem::path parent = m_fileSystem.getParentPath(sourcePath);
        const std::string sourceName = sourcePath.filename().string();
        for (const std::filesystem::path& manifestPath: manifestsInDirectory(parent)) {
            ShaderManifest manifest;
            if (!readShaderManifest(manifestPath, manifest)) {
                continue;
            }
            for (const auto& [type, file]: manifest.stages) {
                if (file.filename().string() == sourceName) {
                    loadShaderResource(manifestPath);
                    break;
                }
            }
        }
    }

    void ShadersLoader::ensureManifestForSource(const std::filesystem::path& sourcePath) const {
        const std::filesystem::path parent = m_fileSystem.getParentPath(sourcePath);
        const std::string name = shaderNameFromPath(sourcePath);
        const std::filesystem::path manifestPath = parent / (name + resourceExtension());

        ShaderManifest manifest;
        if (m_fileSystem.status(manifestPath).exists) {
            readShaderManifest(manifestPath, manifest);
        }
        if (manifest.name.empty()) {
            manifest.name = name;
        }
        manifest.stages[shaderTypeFromPath(sourcePath)] = sourcePath.filename();

        if (writeShaderManifest(manifestPath, manifest.name, manifest.stages)) {
            loadShaderResource(manifestPath);
        }
    }

    std::vector<std::filesystem::path> ShadersLoader::manifestsInDirectory(const std::filesystem::path& directory) const {
        std::vector<std::filesystem::path> manifests;
        for (const std::filesystem::path& file: m_fileSystem.list(directory, false)) {
            if (m_fileSystem.status(file).extension == resourceExtension()) {
                manifests.push_back(file);
            }
        }
        return manifests;
    }

    ShaderType ShadersLoader::shaderTypeFromPath(const std::filesystem::path& path) const {
        if (const StageSuffix* stage = matchStageSuffix(toLower(path.stem().string()))) {
            return stage->type;
        }

        TE_LOGGER_WARN("[ShadersLoader] Could not infer shader stage from '{0}', defaulting to Vertex", path.string());
        return ShaderType::Vertex;
    }

    std::string ShadersLoader::shaderNameFromPath(const std::filesystem::path& path) const {
        const std::string stem = path.stem().string();
        if (const StageSuffix* stage = matchStageSuffix(toLower(stem))) {
            return stem.substr(0, stem.size() - stage->suffix.size());
        }
        return stem;
    }
}
