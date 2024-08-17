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

    void ProjectManager::exportProject(const std::filesystem::path& path) {

    }

    void ProjectManager::createDefaultProject() {
        std::filesystem::create_directory(m_projectPath);
        TE_LOGGER_INFO("New project created at: " + m_projectPath.string());
        std::ofstream fout(m_projectPath.string() + "\\New Project.teproj");
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project Name" << YAML::Value << "New Project";
        out << YAML::EndSeq;
        out << YAML::EndMap;
        fout << out.c_str();
        fout.close();
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
