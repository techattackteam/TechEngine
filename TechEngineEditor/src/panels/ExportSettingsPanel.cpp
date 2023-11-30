#include <imgui.h>
#include "ExportSettingsPanel.hpp"
#include "scene/SceneManager.hpp"
#include "PanelsManager.hpp"
#include <filesystem>
#include <yaml-cpp/emitter.h>
#include <iostream>
#include <fstream>
#include <imgui_internal.h>

namespace TechEngine {

    ExportSettingsPanel::ExportSettingsPanel(PanelsManager &panelsManager, ProjectManager &projectManager, SceneManager &sceneManager) :
            panelsManager(panelsManager),
            projectManager(projectManager),
            sceneManager(sceneManager),
            Panel("ExportSettingsPanel") {

    }

    ExportSettingsPanel::~ExportSettingsPanel() {
    }

    void ExportSettingsPanel::onUpdate() {
        if (visible) {

            ImGui::Begin("Export Settings");
            ImGui::Text("Resolution:");

            ImGui::Text("   Width ");
            ImGui::SameLine();
            ImGui::DragInt("##Width", &width, 1, 0, 0);
            ImGui::Text("   Height ");
            ImGui::SameLine();
            ImGui::DragInt("##Height", &height, 1, 0, 0);


            if (ImGui::Button("Export")) {
                exportProject();
            }
            if (ImGui::Button("Close")) {
                setVisibility(false);
            };

            ImGui::End();
        }
    }

    void ExportSettingsPanel::exportProject() {
        std::filesystem::remove_all(projectManager.getBuildPath());
        std::filesystem::create_directory(projectManager.getBuildPath());
        sceneManager.saveCurrentScene();
        std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                    std::filesystem::copy_options::overwrite_existing;

        //serializeEngineSettings(projectManager.getEngineExportSettingsFile());
        panelsManager.compileUserScripts();
        std::filesystem::copy(projectManager.getUserProjectScenePath(), projectManager.getBuildPath(), copyOptions);
        std::filesystem::copy(projectManager.getUserScriptsDLLPath(), projectManager.getBuildPath(), copyOptions);
        std::filesystem::copy(projectManager.getResourcesPath(), projectManager.getBuildResourcesPath(), copyOptions);
        std::filesystem::copy(projectManager.getRuntimePath(), projectManager.getBuildPath(), copyOptions);
        std::cout << "Export completed!" << std::endl;
    }

    void ExportSettingsPanel::serializeEngineSettings(const std::filesystem::path &exportPath) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Project Name" << "PLACEHOLDER";

        out << YAML::Key << "Window settings";
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << "PLACEHOLDER";
        out << YAML::Key << "width" << YAML::Value << width;
        out << YAML::Key << "height" << YAML::Value << height;
        out << YAML::EndMap;

        out << YAML::Key << "Default Scene" << YAML::Value << "defaultScene";
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(exportPath);
        fout << out.c_str();
    }

    void ExportSettingsPanel::setVisibility(bool visible) {
        this->visible = visible;
    }

    bool ExportSettingsPanel::isVisible() {
        return visible;
    }


}