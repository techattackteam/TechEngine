#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <project/ProjectError.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class ComponentRegistry;
    class ResourceCache;
    struct AssetRecord;
    enum class Role : std::uint8_t;

    class Project {
    private:
        std::filesystem::path m_root;
        std::string m_name;
        ResourceId m_clientStartupScene;

    public:
        std::error_code load(const FileAccess& files, std::string_view manifestPath);

        std::error_code save(FileAccess& files, std::string_view manifestPath) const;

        std::error_code exportTo(Role target, const AssetIndex& index, const FileAccess& files, ComponentRegistry& registry, const ResourceCache& resources, const std::filesystem::path& runtimePath, const std::filesystem::path& exportFolder) const;

        const std::filesystem::path& root() const;

        const std::string& name() const;

        ResourceId getExportStartupScene(Role target) const;

        void setExportStartupScene(Role target, ResourceId id);

    private:
        // payloads holds each reached asset's bytes in out's order, with the scene's left empty.
        std::error_code collectReachedAssets(
            const AssetRecord& scene, std::span<const std::byte> sceneBytes, const AssetIndex& index, const FileAccess& files, ComponentRegistry& registry, const ResourceCache& resources, std::vector<AssetRecord>& out, std::vector<std::vector<std::byte>>& payloads) const;

        std::error_code replaceRuntime(FileAccess& output, const std::string& projectFolder, const std::string& runtimeName) const;
    };
}
