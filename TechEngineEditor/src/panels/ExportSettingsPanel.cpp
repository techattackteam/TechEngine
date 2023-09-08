#include <imgui.h>
#include "ExportSettingsPanel.hpp"
#include "scene/SceneSerializer.hpp"
#include "PanelsManager.hpp"
#include <filesystem>
#include <yaml-cpp/emitter.h>
#include <iostream>
#include <fstream>
#include <imgui_internal.h>
#include "project/ProjectManager.hpp"

namespace TechEngine {

    ExportSettingsPanel::ExportSettingsPanel(std::string &currentScenePath) : currentScenePath(currentScenePath), Panel("ExportSettingsPanel") {

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
        std::filesystem::remove_all(ProjectManager::getBuildPath());
        std::filesystem::create_directory(ProjectManager::getBuildPath());
        if (currentScenePath.empty()) {
            SceneSerializer::serialize(ProjectManager::getUserProjectScenePath().string() + "/defaultScene.scene");
            currentScenePath = "scenes/defaultScene.scene";
        }
        std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                    std::filesystem::copy_options::overwrite_existing;

        //serializeEngineSettings(ProjectManager::getEngineExportSettingsFile());
        PanelsManager::compileUserScripts();
        std::filesystem::copy(ProjectManager::getUserProjectScenePath(), ProjectManager::getBuildPath(), copyOptions);
        std::filesystem::copy(ProjectManager::getUserScriptsDLLPath(), ProjectManager::getBuildPath(), copyOptions);
        std::filesystem::copy(ProjectManager::getResourcesPath(), ProjectManager::getBuildResourcesPath(), copyOptions);
        std::filesystem::copy(ProjectManager::getRuntimePath(), ProjectManager::getBuildPath(), copyOptions);
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