#include <imgui.h>
#include "ExportSettingsPanel.hpp"
#include "PanelsManager.hpp"
#include <filesystem>
#include <yaml-cpp/emitter.h>
#include <iostream>
#include <fstream>
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    ExportSettingsPanel::ExportSettingsPanel(PanelsManager& panelsManager, ProjectManager& projectManager, SceneManager& sceneManager, ShadersManager& shadersManager) : panelsManager(panelsManager),
                                                                                                                                                                         projectManager(projectManager),
                                                                                                                                                                         sceneManager(sceneManager),
                                                                                                                                                                         shadersManager(shadersManager),
                                                                                                                                                                         Panel("ExportSettingsPanel") {
        m_open = false;
    }

    ExportSettingsPanel::~ExportSettingsPanel() {
    }

    void ExportSettingsPanel::onUpdate() {
        ImGui::Text("Resolution:");

        ImGui::Text("   Width ");
        ImGui::SameLine();
        ImGui::DragInt("##Width", &width, 1, 0, 0);
        ImGui::Text("   Height ");
        ImGui::SameLine();
        ImGui::DragInt("##Height", &height, 1, 0, 0);


        if (ImGui::Button("Export")) {
            exportGameProject();
        }
        if (ImGui::Button("Close")) {
            m_open = false;
        };
    }

    void ExportSettingsPanel::exportGameProject() {
        try {
            std::filesystem::remove_all(projectManager.getProjectGameExportPath());
            std::filesystem::create_directory(projectManager.getProjectGameExportPath());
            sceneManager.saveCurrentScene();
            std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                        std::filesystem::copy_options::overwrite_existing;

            std::filesystem::copy(projectManager.getProjectFilePath(), projectManager.getProjectGameExportPath(), copyOptions);
            panelsManager.compileUserScripts(RELEASE);
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath(), projectManager.getProjectGameExportPath().string() + "//Assets", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::create_directory(projectManager.getProjectGameExportPath().string() + "//Resources");
            FileSystem::copyRecursive(projectManager.getProjectResourcesPath(), projectManager.getProjectGameExportPath().string() + "//Resources", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::copy(FileSystem::runtimePath, projectManager.getProjectGameExportPath(), copyOptions);
            TE_LOGGER_INFO("Project exported to: {0}", projectManager.getProjectGameExportPath().string());
        } catch (std::filesystem::filesystem_error& e) {
            TE_LOGGER_ERROR("Error while exporting project: {0}", e.what());
            return;
        }
    }

    void ExportSettingsPanel::exportServerProject() {
        try {
            std::filesystem::remove_all(projectManager.getProjectServerExportPath());
            std::filesystem::create_directory(projectManager.getProjectServerExportPath());
            sceneManager.saveCurrentScene();
            std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                        std::filesystem::copy_options::overwrite_existing;

            std::filesystem::copy(projectManager.getProjectFilePath(), projectManager.getProjectServerExportPath(), copyOptions);
            panelsManager.compileUserScripts(RELEASE);
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath(), projectManager.getProjectServerExportPath().string() + "//Assets", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::create_directory(projectManager.getProjectServerExportPath().string() + "//Resources");
            FileSystem::copyRecursive(projectManager.getProjectResourcesPath(), projectManager.getProjectServerExportPath().string() + "//Resources", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::copy(FileSystem::serverPath, projectManager.getProjectServerExportPath(), copyOptions);
            TE_LOGGER_INFO("Project exported to: {0}", projectManager.getProjectServerExportPath().string());
        } catch (std::filesystem::filesystem_error& e) {
            TE_LOGGER_ERROR("Error while exporting project: {0}", e.what());
            return;
        }
    }

    void ExportSettingsPanel::serializeEngineSettings(const std::filesystem::path& exportPath) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project Name" << projectManager.getProjectName();

        out << YAML::Key << "Window settings";
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << projectManager.getProjectName();
        out << YAML::Key << "width" << YAML::Value << width;
        out << YAML::Key << "height" << YAML::Value << height;
        out << YAML::EndMap;

        out << YAML::Key << "Default Scene" << YAML::Value << sceneManager.getActiveSceneName();
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(exportPath);
        fout << out.c_str();
    }
}
