#include <imgui.h>
#include "project/ProjectManager.hpp"
#include "ProjectBrowserPanel.hpp"
#include "core/Logger.hpp"
#include "imgui_stdlib.h"
#include "UIUtils/ImGuiUtils.hpp"
#include "scene/SceneManager.hpp"

namespace TechEngine {
    ProjectBrowserPanel::ProjectBrowserPanel() : Panel("ProjectBrowserPanel") {

    }

    void ProjectBrowserPanel::init() {
        currentPath = ProjectManager::getUserProjectRootPath();
    }

    void ProjectBrowserPanel::onUpdate() {
        menuWindowOpen = false;
        ImGui::Begin("Project Browser");
        if (currentPath != std::filesystem::path(ProjectManager::getUserProjectRootPath())) {
            if (ImGui::Button("...")) {
                currentPath = currentPath.parent_path();
            }
        }

        for (auto &directoryEntry: std::filesystem::directory_iterator(currentPath)) {
            const auto &path = directoryEntry.path();
            auto relativePath = std::filesystem::relative(path, ProjectManager::getUserProjectRootPath());
            std::string filenameString = relativePath.filename().string();
            if (directoryEntry.is_directory()) {
                if (ImGui::Button(filenameString.c_str())) {
                    currentPath /= path.filename();
                }
                if (ImGui::BeginPopupContextItem()) {
                    menuWindowOpen = true;
                    if (ImGui::Button("Open")) {
                        currentPath /= path.filename();
                    }
                    std::string newName;
                    if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "New name", newName)) {
                        renameFile(path.filename().string(), newName);
                    }
                    if (ImGui::Button("Delete")) {
                        std::filesystem::remove_all(path);
                    }
                    ImGui::EndPopup();

                }
            } else {
                if (ImGui::Button(filenameString.c_str())) {
                    openFile(filenameString);
                }
                if (ImGui::BeginPopupContextItem()) {
                    menuWindowOpen = true;
                    if (ImGui::Button("Open")) {
                        openFile(filenameString);
                    }
                    std::string newName;
                    if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "New name", newName)) {
                        renameFile(path.filename().string(), newName);
                    }
                    if (ImGui::Button("Delete")) {
                        std::filesystem::remove_all(path);
                    }
                    ImGui::EndPopup();
                }
            }
        }

        if (!menuWindowOpen && ImGui::BeginPopupContextWindow(0, 1)) {
            std::string newFolderName;
            if (ImGuiUtils::beginMenuWithInputMenuField("New Folder", "New Folder", newFolderName)) {
                std::filesystem::create_directory(currentPath / newFolderName);
            }
            std::string newSceneName;
            if (ImGuiUtils::beginMenuWithInputMenuField("New Scene", "Scene name", newSceneName)) {
                std::string path = currentPath.string() + "\\" + newSceneName + ".scene";
                SceneManager::createNewScene(path);
            }
            std::string newScriptName;
            if (ImGuiUtils::beginMenuWithInputMenuField("New Script", "Script name", newScriptName)) {
                TE_LOGGER_INFO("New script name: {0}", newScriptName);
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void ProjectBrowserPanel::renameFile(const std::string &filename, const std::string &newName) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        std::filesystem::rename(currentPath / filename, currentPath / (newName + extension));
    }

    void ProjectBrowserPanel::openFile(const std::string &filename) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        if (extension == ".scene") {
            //remove extension from file
            std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
            SceneManager::loadScene(filenameWithoutExtension);
        }
    }
}
