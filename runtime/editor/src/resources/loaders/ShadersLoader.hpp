#pragma once
#include "resources/IResourceLoader.hpp"
#include "resources/ResourceSystem.hpp"
#include "resources/shader/ShaderResource.hpp"
#include "fileSystem/FileSystem.hpp"
#include "TechEngine/core/fileSystem/IFileWatcher.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    // A shader program is authored as a .teshader manifest that gives the program an explicit name
    // (the string the renderer looks up) plus the .glsl source file backing each stage. When source
    // files exist without a manifest, the loader generates one automatically so scattered stages get
    // grouped into a single, linkable program.
    class ShadersLoader : public IResourceLoader {
    private:
        struct ShaderManifest {
            std::string name;
            // Stage -> source file name, stored relative to the manifest so the program is portable.
            std::map<ShaderType, std::filesystem::path> stages;
        };

        ResourceSystem& m_resourcesSystem;
        FileSystem& m_fileSystem;
        IFileWatcher& m_fileWatcher;
        uint32_t m_subscriptionId = 0;

        // Manifest virtual path -> program name, so a deleted manifest can be unregistered even
        // though its file (and thus its explicit name) is already gone.
        mutable std::unordered_map<std::string, std::string> m_manifestPrograms;

    public:
        ShadersLoader(ResourceSystem& resourcesSystem, FileSystem& fileSystem, IFileWatcher& fileWatcher);

        ~ShadersLoader() override;

        // Groups source files that aren't part of any manifest yet and writes a .teshader for each
        // new program. Returns the manifests that were created so the caller can load them.
        std::vector<std::filesystem::path> generateMissingShaderManifests(
            const std::vector<std::filesystem::path>& sourceFiles,
            const std::vector<std::filesystem::path>& existingManifests) const;

        // Builds and (re)registers a ShaderResource from a .teshader manifest.
        std::shared_ptr<ShaderResource> loadShaderResource(const std::filesystem::path& manifestPath) const;

        // Writes a .teshader manifest with an explicit program name and its stage source files.
        bool writeShaderManifest(const std::filesystem::path& manifestPath,
                                 const std::string& name,
                                 const std::map<ShaderType, std::filesystem::path>& stages) const;

        // Explicit/editor-driven creation: authors a manifest next to its sources and loads it.
        std::shared_ptr<ShaderResource> createShaderResource(const std::filesystem::path& manifestPath,
                                                             const std::string& name,
                                                             const std::map<ShaderType, std::filesystem::path>& stages) const;

        void removeShaderResource(const std::filesystem::path& manifestPath) const;

        std::vector<std::string> sourceExtensions() const override;

        std::string resourceExtension() const override;

    private:
        void onFileChanged(const FileChangedEvent& event) const;

        bool readShaderManifest(const std::filesystem::path& manifestPath, ShaderManifest& outManifest) const;

        // Reloads every manifest in the source's directory that references it (hot reload).
        void onSourceChanged(const std::filesystem::path& sourcePath) const;

        // Creates or extends the base-name manifest that owns a newly discovered source file.
        void ensureManifestForSource(const std::filesystem::path& sourcePath) const;

        std::vector<std::filesystem::path> manifestsInDirectory(const std::filesystem::path& directory) const;

        ShaderType shaderTypeFromPath(const std::filesystem::path& path) const;

        // Program name shared by every stage file, i.e. the file stem without its stage suffix
        // ("geometryVertex" / "geometryFragment" -> "geometry"). Used as the default manifest name.
        std::string shaderNameFromPath(const std::filesystem::path& path) const;
    };
}
