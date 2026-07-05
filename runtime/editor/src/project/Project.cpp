#include "Project.hpp"

#include "core/Logger.hpp"
#include "resources/ResourceSystem.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    Project::Project() {
    }

    void Project::init(const std::filesystem::path& projectPath) {
        m_projectPath = projectPath;
        m_paths[translatePathType(PathType::Assets, AppType::Client)] = m_projectPath / "assets/client";
        m_paths[translatePathType(PathType::Assets, AppType::Common)] = m_projectPath / "assets/common";
        m_paths[translatePathType(PathType::Assets, AppType::Server)] = m_projectPath / "assets/server";

        m_paths[translatePathType(PathType::Resources, AppType::Client)] = m_projectPath / "resources/client";
        m_paths[translatePathType(PathType::Resources, AppType::Common)] = m_projectPath / "resources/core";
        m_paths[translatePathType(PathType::Resources, AppType::Server)] = m_projectPath / "resources/server";

        m_paths[translatePathType(PathType::Cache, AppType::Client)] = m_projectPath / "cache/client";
        m_paths[translatePathType(PathType::Cache, AppType::Common)] = m_projectPath / "cache/common";
        m_paths[translatePathType(PathType::Cache, AppType::Server)] = m_projectPath / "cache/server";
    }

    void Project::loadProject(std::filesystem::path& projectPath, YAML::Node& config) {
        if (!config[projectConfigToString(ProjectConfig::Scene)].IsDefined()) {
            TE_LOGGER_ERROR("Last Loaded Scene not found in project config");
            config[projectConfigToString(ProjectConfig::Scene)] = "DefaultScene";
        }
        m_projectConfigs[ProjectConfig::ProjectPath] = projectPath.string();
        m_projectConfigs[ProjectConfig::Scene] = config[projectConfigToString(ProjectConfig::Scene)].as<std::string>();
    }

    /*void Project::loadRuntimeProject(std::filesystem::path& projectPath) {
        std::vector<std::filesystem::path> paths = m_systemsRegistry.getSystem<FileSystem>().list(projectPath, false);
        if (paths.empty()) {
            TE_LOGGER_ERROR("Project config file not found in path: {}", projectPath.string());
            return;
        }
        YAML::Node config = YAML::LoadFile(paths[0].string());
        m_projectConfigs[ProjectConfig::ProjectName] = config[projectConfigToString(ProjectConfig::ProjectName)].as<std::string>();
        loadProject(projectPath, config);
    }*/

    YAML::Node Project::saveProject() {
        YAML::Node config;

        config[projectConfigToString(ProjectConfig::Scene)] = m_projectConfigs[ProjectConfig::Scene];
        return config;
    }

    const std::unordered_map<ProjectConfig, std::string>& Project::getProjectConfigs() const {
        return m_projectConfigs;
    }

    const std::filesystem::path& Project::getRootPath() const {
        return m_projectPath;
    }

    const std::filesystem::path& Project::getPath(PathType pathType, AppType appType) {
        return m_paths.at(translatePathType(pathType, appType));
    }

    std::string Project::projectConfigToString(ProjectConfig projectConfig) {
        switch (projectConfig) {
            case ProjectConfig::ProjectName:
                return "Project Name";
            case ProjectConfig::ProjectPath:
                return "Project Path";
            case ProjectConfig::Scene:
                return "Last Loaded Scene";
            default:
                return "";
        }
    }

    int Project::translatePathType(PathType pathType, AppType appType) {
        return static_cast<int>(pathType) * 10 + static_cast<int>(appType);
    }
}
