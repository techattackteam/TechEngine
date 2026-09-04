#pragma once

#include <TechEngine/platform/files/FileAccess.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace TechEngine {
    enum class ProjectResult : std::uint8_t { Ok, ReadFailed, WriteFailed, ParseFailed, SchemaInvalid };

    class Project {
    private:
        std::filesystem::path m_root;
        std::string m_name;

    public:
        ProjectResult load(const FileAccess& files, std::string_view manifestPath);

        ProjectResult save(FileAccess& files, std::string_view manifestPath) const;

        const std::filesystem::path& root() const;

        const std::string& name() const;
    };
}
