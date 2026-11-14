#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace TechEngineTests {
    class ScratchDirectory {
    public:
        explicit ScratchDirectory(std::string_view name) : m_root(processRoot() / name) {
            std::filesystem::remove_all(m_root);
            std::filesystem::create_directories(m_root);
        }

        ~ScratchDirectory() {
            std::error_code ec;
            std::filesystem::remove_all(m_root, ec);
            std::filesystem::remove(m_root.parent_path(), ec);
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
        // ctest runs cases in parallel, and the constructor deletes whatever is at its path, so
        // two processes that share a name must never share a folder.
        static std::filesystem::path processRoot() {
#if defined(_WIN32)
            const int id = _getpid();
#else
            const pid_t id = getpid();
#endif
            return std::filesystem::temp_directory_path() / "TechEngineTests" / std::to_string(id);
        }

        std::filesystem::path m_root;
    };
}
