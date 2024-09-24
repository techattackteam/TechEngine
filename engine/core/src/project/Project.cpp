#include "Project.hpp"

#include "core/Logger.hpp"
#include "files/FileUtils.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    Project::Project(const std::filesystem::path& projectPath) {
        m_projectPath = projectPath;
    }

    void Project::init() {
        m_paths[translatePathType(PathType::Assets, AppType::Client)] = m_projectPath / "assets/client";
        m_paths[translatePathType(PathType::Assets, AppType::Common)] = m_projectPath / "assets/common";
        m_paths[translatePathType(PathType::Assets, AppType::Server)] = m_projectPath / "assets/server";

        m_paths[translatePathType(PathType::Resources, AppType::Client)] = m_projectPath / "resources/client";
        m_paths[translatePathType(PathType::Resources, AppType::Common)] = m_projectPath / "resources/common";
        m_paths[translatePathType(PathType::Resources, AppType::Server)] = m_projectPath / "resources/server";

        m_paths[translatePathType(PathType::Cache, AppType::Client)] = m_projectPath / "cache/client";
        m_paths[translatePathType(PathType::Cache, AppType::Common)] = m_projectPath / "cache/common";
        m_paths[translatePathType(PathType::Cache, AppType::Server)] = m_projectPath / "cache/server";
    }

    void Project::loadProject(std::filesystem::path& projectPath, YAML::Node& config) {
        if (!config["Last Loaded Scene"].IsDefined()) {
            TE_LOGGER_ERROR("Last Loaded Scene not found in project config");
            config["Last Loaded Scene"] = "DefaultScene";
        }
        //m_projectConfigs[ProjectConfig::ProjectName] = config["Project Name"].as<std::string>();
        m_projectConfigs[ProjectConfig::ProjectPath] = projectPath.string();
        m_projectConfigs[ProjectConfig::Scene] = config["Last Loaded Scene"].as<std::string>();
    }

    void Project::loadRuntimeProject(std::filesystem::path& projectPath) {
        std::vector<std::string> paths = FileUtils::getAllFilesWithExtension({projectPath.string()}, {".teproj"}, true);
        YAML::Node config = YAML::LoadFile(paths[0]);
        loadProject(projectPath, config);
    }

    YAML::Node Project::saveProject() {
        YAML::Node config;
        config["Last Loaded Scene"] = m_projectConfigs[ProjectConfig::Scene];

        return config;
    }

    const std::unordered_map<ProjectConfig, std::string>& Project::getProjectConfigs() {
        return m_projectConfigs;
    }

    const std::filesystem::path& Project::getRootPath() {
        return m_projectPath;
    }

    const std::filesystem::path& Project::getPath(PathType pathType, AppType appType) {
        return m_paths.at(translatePathType(pathType, appType));
    }

    int Project::translatePathType(PathType pathType, AppType appType) {
        return static_cast<int>(pathType) * 10 + static_cast<int>(appType);
    }
}
