#pragma once

#include <TechEngine/platform/files/FileAccess.hpp>

#include <project/ProjectError.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace TechEngine {
    class Project {
    private:
        std::filesystem::path m_root;
        std::string m_name;

    public:
        std::error_code load(const FileAccess& files, std::string_view manifestPath);

        std::error_code save(FileAccess& files, std::string_view manifestPath) const;

        const std::filesystem::path& root() const;

        const std::string& name() const;
    };
}
