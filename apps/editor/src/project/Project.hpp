#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <project/ProjectError.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace TechEngine {
    class AssetIndex;
    enum class Role : std::uint8_t;

    class Project {
    private:
        std::filesystem::path m_root;
        std::string m_name;
        ResourceId m_clientStartupScene;

    public:
        std::error_code load(const FileAccess& files, std::string_view manifestPath);

        std::error_code save(FileAccess& files, std::string_view manifestPath) const;

        std::error_code exportTo(Role target, const AssetIndex& index, const FileAccess& files, const std::filesystem::path& folder) const;

        const std::filesystem::path& root() const;

        const std::string& name() const;

        ResourceId exportStartupScene(Role target) const;

        void setExportStartupScene(Role target, ResourceId id);
    };
}
