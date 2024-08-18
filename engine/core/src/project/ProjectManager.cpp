#include "ProjectManager.hpp"
#include "core/Logger.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace TechEngine {
    ProjectManager::ProjectManager(const std::filesystem::path& projectPath) : m_projectPath(projectPath) {
    }

    void ProjectManager::init() {
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

    void ProjectManager::exportProject(const std::filesystem::path& path, ProjectType projectType) {
        std::string exportPath = path.string() + "\\" + m_projectName.string();
        if (!std::filesystem::exists(exportPath)) {
            std::filesystem::create_directory(exportPath);
        } else {
            for (const auto& entry: std::filesystem::directory_iterator(exportPath)) {
                std::filesystem::remove_all(entry.path());
            }
        }
        std::filesystem::create_directory(exportPath + "\\assets");
        std::filesystem::create_directory(exportPath + "\\assets\\common");
        std::filesystem::create_directory(exportPath + "\\resources");
        std::filesystem::create_directory(exportPath + "\\resources\\common");
        std::filesystem::copy(m_projectPath.string() + "\\assets\\common", exportPath + "\\assets\\common", std::filesystem::copy_options::recursive);
        std::filesystem::copy(m_projectPath.string() + "\\resources\\common", exportPath + "\\resources\\common", std::filesystem::copy_options::recursive);
        if (projectType == ProjectType::CLIENT) {
            std::filesystem::create_directory(exportPath + "\\assets\\client");
            std::filesystem::create_directory(exportPath + "\\resources\\client");
            std::filesystem::copy(m_projectPath.string() + "\\assets\\client", exportPath + "\\assets\\client", std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\resources\\client", exportPath + "\\resources\\client", std::filesystem::copy_options::recursive);

            //Copy runtime files to project folder
            std::filesystem::copy(m_clientRuntimePath, exportPath, std::filesystem::copy_options::recursive);
        } else if (projectType == ProjectType::SERVER) {
            std::filesystem::create_directory(exportPath + "\\assets\\server");
            std::filesystem::create_directory(exportPath + "\\resources\\server");
            std::filesystem::copy(m_projectPath.string() + "\\assets\\server", exportPath + "\\assets\\server", std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\resources\\server", exportPath + "\\resources\\server", std::filesystem::copy_options::recursive);

            //Copy runtime files to project folder
            std::filesystem::copy(m_serverRuntimesPath, exportPath, std::filesystem::copy_options::recursive);
        }
        std::filesystem::copy(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj", exportPath + "\\" + m_projectName.string() + ".teproj", std::filesystem::copy_options::recursive);
        TE_LOGGER_INFO("Project exported to: " + exportPath);
    }

    void ProjectManager::createProject(const std::string& projectName) {
        std::filesystem::create_directory(m_projectPath);
        TE_LOGGER_INFO("New project created at: " + m_projectPath.string());
        std::filesystem::create_directory(m_projectPath.string() + "\\assets");
        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\server");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\server");
        std::filesystem::copy(std::filesystem::current_path().string() + "\\resources\\project\\libs", m_projectPath.string() + "\\resources\\common\\libs", std::filesystem::copy_options::recursive);
        std::ofstream fout(m_projectPath.string() + "\\" + projectName + ".teproj");
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project Name" << YAML::Value << projectName;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        fout << out.c_str();
        fout.close();
    }

    void ProjectManager::createDefaultProject() {
        createProject("New Project");
    }

    void ProjectManager::loadProject() {
        if (!std::filesystem::exists(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj")) {
            TE_LOGGER_ERROR("Project config not found: " + m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
            return;
        }
        YAML::Node config = YAML::LoadFile(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
        if (!config["Project Name"].IsDefined()) {
            TE_LOGGER_ERROR("Project Name not found in project config");
            config["Project Name"] = "New Project";
        }
        TE_LOGGER_INFO("Project loaded: " + m_projectName.string());
    }
}
