#pragma once
#include <filesystem>
#include <unordered_map>

#include "systems/System.hpp"

namespace TechEngine {
    enum class PathType {
        Assets,
        Resources,
        Cache,
    };

    enum class AppType {
        Common,
        Client,
        Server,
    };

    class PathsBank : public System {
    private:
        std::filesystem::path m_rootPath;
        std::unordered_map<int, std::filesystem::path> m_paths;

    public:
        explicit PathsBank(std::filesystem::path rootPath);

        void init() override;

        void shutdown() override;

        const std::filesystem::path& getRootPath() {
            return m_rootPath;
        }

        const std::filesystem::path& getPath(PathType pathType, AppType appType);

    private:
        int translatePathType(PathType pathType, AppType appType);
    };
}
