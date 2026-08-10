#pragma once

#include <filesystem>
#include <fstream>
#include <string_view>

namespace TechEngineTests {
    class ScratchDirectory {
    public:
        explicit ScratchDirectory(std::string_view name) : m_root{std::filesystem::temp_directory_path() / "TechEngineTests" / name} {
            std::filesystem::remove_all(m_root);
            std::filesystem::create_directories(m_root);
        }

        ~ScratchDirectory() {
            std::error_code ec;
            std::filesystem::remove_all(m_root, ec);
        }

        ScratchDirectory(const ScratchDirectory&) = delete;

        ScratchDirectory& operator=(const ScratchDirectory&) = delete;

        const std::filesystem::path& root() const {
            return m_root;
        }

        std::filesystem::path writeFile(const std::filesystem::path& relative, std::string_view contents) const {
            const std::filesystem::path target = m_root / relative;
            std::filesystem::create_directories(target.parent_path());
            std::ofstream file{target, std::ios::binary | std::ios::trunc};
            file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            return target;
        }

        std::filesystem::path makeDirectory(const std::filesystem::path& relative) const {
            const std::filesystem::path target = m_root / relative;
            std::filesystem::create_directories(target);
            return target;
        }

    private:
        std::filesystem::path m_root;
    };
}
