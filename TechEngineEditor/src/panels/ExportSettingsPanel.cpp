#include <imgui.h>
#include "ExportSettingsPanel.hpp"
#include "scene/SceneManager.hpp"
#include "PanelsManager.hpp"
#include <filesystem>
#include <yaml-cpp/emitter.h>
#include <iostream>
#include <fstream>
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"

namespace TechEngine {

    ExportSettingsPanel::ExportSettingsPanel(PanelsManager &panelsManager, ProjectManager &projectManager, SceneManager &sceneManager, ShadersManager &shadersManager) :
            panelsManager(panelsManager),
            projectManager(projectManager),
            sceneManager(sceneManager),
            shadersManager(shadersManager),
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
        std::filesystem::remove_all(projectManager.getProjectExportPath());
        std::filesystem::create_directory(projectManager.getProjectExportPath());
        sceneManager.saveCurrentScene();
        std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                    std::filesystem::copy_options::overwrite_existing;

        serializeEngineSettings(projectManager.getProjectExportPath().string() + "//Export.texp");
        std::filesystem::copy(projectManager.getProjectFilePath(), projectManager.getProjectExportPath(), copyOptions);
        panelsManager.compileUserScripts(RELEASE);
        FileSystem::copyRecursive(projectManager.getProjectAssetsPath(), projectManager.getProjectExportPath().string() + "//Assets", {".cpp", ".hpp"}, {"cmake"});
        if (std::filesystem::exists(projectManager.getScriptsDebugDLLPath())) {
            std::filesystem::copy(projectManager.getScriptsDebugDLLPath(), projectManager.getProjectExportPath(), copyOptions);
        }
        std::filesystem::create_directory(projectManager.getProjectExportPath().string() + "//Resources");
        FileSystem::copyRecursive(projectManager.getProjectResourcesPath(), projectManager.getProjectExportPath().string() + "//Resources", {".cpp", ".hpp"}, {"cmake"});
        std::filesystem::copy(FileSystem::runtimePath, projectManager.getProjectExportPath(), copyOptions);
        //shadersManager.exportShaderFiles(projectManager.getProjectExportPath().string() + "//Resources//shaders");
        TE_LOGGER_INFO("Project exported to: {0}", projectManager.getProjectExportPath().string());
    }

    void ExportSettingsPanel::serializeEngineSettings(const std::filesystem::path &exportPath) {
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

    void ExportSettingsPanel::setVisibility(bool visible) {
        this->visible = visible;
    }

    bool ExportSettingsPanel::isVisible() {
        return visible;
    }


}