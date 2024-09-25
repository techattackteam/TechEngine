#include "ProjectManager.hpp"
#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include "files/FileUtils.hpp"
#include "project/Project.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    ProjectManager::ProjectManager(SystemsRegistry& clientSystemsRegistry,
                                   SystemsRegistry& serverSystemsRegistry) : m_systemsRegistry(clientSystemsRegistry),
                                                                             m_clientSystemsRegistry(clientSystemsRegistry),
                                                                             m_serverSystemsRegistry(serverSystemsRegistry) {
    }

    void ProjectManager::init(const std::filesystem::path& projectPath) {
        m_projectPath = projectPath;
        if (!std::filesystem::exists(m_projectPath)) {
            TE_LOGGER_ERROR("Project not found: " + m_projectPath.string());
            createDefaultProject();
        }
        for (auto& entry: std::filesystem::directory_iterator(m_projectPath)) {
            if (entry.path().extension() == ".teproj") {
                m_projectName = entry.path().stem();
                break;
            }
        }
        loadProject();
    }

    void ProjectManager::shutdown() {
    }

    void ProjectManager::createProject(const std::string& projectName) {
        assert(!projectName.empty());
        std::filesystem::create_directory(m_projectPath);
        std::filesystem::create_directory(m_projectPath.string() + "\\assets");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache");
        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\common");

        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\client");

        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\server");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\server");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\server");

        TE_LOGGER_INFO("New project created at: " + m_projectPath.string());
        std::filesystem::copy(std::filesystem::current_path().string() + "\\resources\\templates\\project", m_projectPath.string(), std::filesystem::copy_options::recursive);
        std::ofstream fout(m_projectPath.string() + "\\" + projectName + ".teproj");
        YAML::Node config;
        config["Project Name"] = projectName;

        YAML::Node client;
        client["Last Loaded Scene"] = "DefaultScene";
        config["Client"] = client;

        YAML::Node server;
        server["Last Loaded Scene"] = "DefaultScene";
        config["Server"] = server;

        fout << config;
        fout.close();
    }

    void ProjectManager::exportProject(const std::filesystem::path& path, ProjectType projectType) {
        assert(!path.empty() && !m_projectName.empty() && !m_projectPath.empty());
        std::string exportPath = path.string() + "\\" + m_projectName.string();
        if (!std::filesystem::exists(exportPath)) {
            //Make parent directories

            std::filesystem::create_directories(exportPath);
        } else {
            for (const auto& entry: std::filesystem::directory_iterator(exportPath)) {
                std::filesystem::remove_all(entry.path());
            }
        }
        std::filesystem::create_directory(exportPath + "\\assets");
        std::filesystem::create_directory(exportPath + "\\assets\\common");
        std::filesystem::create_directory(exportPath + "\\resources");
        std::filesystem::create_directory(exportPath + "\\resources\\common");
        std::filesystem::create_directory(exportPath + "\\cache");
        std::filesystem::create_directory(exportPath + "\\cache\\common");
        FileUtils::copyRecursive(m_projectPath.string() + "\\assets\\common", exportPath + "\\assets\\common", {".dll", ".exe", ".cpp", ".hpp"}, {});
        FileUtils::copyRecursive(m_projectPath.string() + "\\resources\\common", exportPath + "\\resources\\common", {}, {"TechEngineAPI"});
        FileUtils::copyRecursive(m_projectPath.string() + "\\cache\\common", exportPath + "\\cache\\common", {}, {"findCmake", "libs"});

        if (projectType == ProjectType::Client) {
            Project& project = m_clientSystemsRegistry.getSystem<Project>();
            project.saveProject();
            std::filesystem::create_directory(exportPath + "\\assets\\client");
            std::filesystem::create_directory(exportPath + "\\resources\\client");
            std::filesystem::create_directory(exportPath + "\\cache\\client");
            FileUtils::copyRecursive(m_projectPath.string() + "\\assets\\client", exportPath + "\\assets\\client", {".dll", ".exe", ".cpp", ".hpp"}, {});
            FileUtils::copyRecursive(m_projectPath.string() + "\\resources\\client", exportPath + "\\resources\\client", {}, {"TechEngineAPI"});
            FileUtils::copyRecursive(m_projectPath.string() + "\\cache\\client", exportPath + "\\cache\\client", {}, {});
            //Copy runtime files to project folder
            std::filesystem::copy(m_clientRuntimePath, exportPath, std::filesystem::copy_options::recursive);
            std::ofstream fout = std::ofstream(exportPath + "\\" + m_projectName.string() + ".teproj");
            YAML::Node config;
            config[project.projectConfigToString(ProjectConfig::ProjectName)] = m_projectName.string();
            for (const auto& entry: project.getProjectConfigs()) {
                config[project.projectConfigToString(entry.first)] = entry.second;
            }

            fout << config;
            fout.close();
        } else if (projectType == ProjectType::Server) {
            Project& project = m_serverSystemsRegistry.getSystem<Project>();
            project.saveProject();
            std::filesystem::create_directory(exportPath + "\\assets\\server");
            std::filesystem::create_directory(exportPath + "\\resources\\server");
            std::filesystem::create_directory(exportPath + "\\cache\\server");
            std::filesystem::copy(m_projectPath.string() + "\\assets\\server", exportPath + "\\assets\\server", std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\resources\\server", exportPath + "\\resources\\server", std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\cache\\server", exportPath + "\\cache\\server", std::filesystem::copy_options::recursive);

            //Copy runtime files to project folder
            std::filesystem::copy(m_serverRuntimesPath, exportPath, std::filesystem::copy_options::recursive);
            std::ofstream fout = std::ofstream(exportPath + "\\" + m_projectName.string() + ".teproj");
            YAML::Node config;
            config[project.projectConfigToString(ProjectConfig::ProjectName)] = m_projectName.string();
            for (const auto& entry: project.getProjectConfigs()) {
                config[project.projectConfigToString(entry.first)] = entry.second;
            }

            fout << config;
            fout.close();
        }
        TE_LOGGER_INFO("Project exported to: " + exportPath);
    }

    void ProjectManager::saveProject() {
        assert(!m_projectName.empty());
        m_clientSystemsRegistry.getSystem<ScenesManager>().saveScene();
        m_serverSystemsRegistry.getSystem<ScenesManager>().saveScene();

        std::ofstream fout(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
        YAML::Node config;
        config["Project Name"] = m_projectName.string();

        YAML::Node client = m_clientSystemsRegistry.getSystem<Project>().saveProject();
        config["Client"] = client;

        YAML::Node server = m_serverSystemsRegistry.getSystem<Project>().saveProject();
        config["Server"] = server;

        fout << config;
        fout.close();
    }

    std::filesystem::path ProjectManager::getCmakeListPath() const {
        return m_assetsPath.string();
    }

    std::filesystem::path ProjectManager::getCmakeBuildPath(CompileMode compileMode) {
        assert(compileMode == CompileMode::Debug || compileMode == CompileMode::Release);
        std::string buildMode;
        if (compileMode == CompileMode::Debug) {
            buildMode = "debug";
        } else if (compileMode == CompileMode::Release) {
            buildMode = "release";
        }

        return m_cachePath.string() + "\\common\\cmake-build-" + buildMode;
    }

    std::filesystem::path ProjectManager::getCmakePath() const {
        return m_cmakePath;
    }

    std::filesystem::path ProjectManager::getProjectPath() const {
        return m_projectPath;
    }

    std::filesystem::path ProjectManager::getAssetsPath() const {
        return m_assetsPath;
    }

    std::filesystem::path ProjectManager::getResourcesPath() const {
        return m_resourcesPath;
    }

    std::filesystem::path ProjectManager::getCachePath() const {
        return m_cachePath;
    }

    std::string ProjectManager::getProjectName() const {
        return m_projectName.string();
    }

    void ProjectManager::createDefaultProject() {
        createProject("New Project");
    }

    void ProjectManager::loadProject() {
        assert(!m_projectName.empty());
        if (!std::filesystem::exists(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj")) {
            TE_LOGGER_ERROR("Project config not found: " + m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
            return;
        }
        YAML::Node config = YAML::LoadFile(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
        if (!config["Project Name"].IsDefined()) {
            TE_LOGGER_ERROR("Project Name not found in project config");
            config["Project Name"] = "New Project";
        }
        YAML::Node clientNode = config["Client"];
        m_clientSystemsRegistry.getSystem<Project>().loadProject(m_projectPath, clientNode);

        YAML::Node serverNode = config["Server"];
        m_serverSystemsRegistry.getSystem<Project>().loadProject(m_projectPath, serverNode);

        //TE_LOGGER_INFO("Project loaded: " + m_projectName.string());
        m_assetsPath = m_projectPath.string() + "\\assets";
        m_resourcesPath = m_projectPath.string() + "\\resources";
        m_cachePath = m_projectPath.string() + "\\cache";
    }
}
