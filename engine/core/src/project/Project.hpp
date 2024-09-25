#pragma once

#include "systems/System.hpp"

#include <filesystem>
#include <unordered_map>
#include <yaml-cpp/node/node.h>

namespace TechEngine {
    enum class ProjectConfig {
        ProjectName,
        ProjectPath,
        Scene,
    };

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

    class CORE_DLL Project : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::filesystem::path m_projectPath;
        std::unordered_map<ProjectConfig, std::string> m_projectConfigs;
        std::unordered_map<int, std::filesystem::path> m_paths;

    public:
        explicit Project(const std::filesystem::path& projectPath, SystemsRegistry& systemsRegistry);

        void init() override;

        void loadProject(std::filesystem::path& projectPath, YAML::Node& config);

        void loadRuntimeProject(std::filesystem::path& projectPath);

        YAML::Node saveProject();

        const std::unordered_map<ProjectConfig, std::string>& getProjectConfigs();

        const std::filesystem::path& getRootPath();

        const std::filesystem::path& getPath(PathType pathType, AppType appType);

        std::string projectConfigToString(ProjectConfig projectConfig);

    private:
        int translatePathType(PathType pathType, AppType appType);
    };
}
