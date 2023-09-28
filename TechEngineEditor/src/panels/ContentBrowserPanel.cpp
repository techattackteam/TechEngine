#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include "imgui_internal.h"
#include "project/ProjectManager.hpp"
#include "ContentBrowserPanel.hpp"
#include "core/Logger.hpp"
#include "imgui_stdlib.h"
#include "UIUtils/ImGuiUtils.hpp"
#include "scene/SceneManager.hpp"

namespace TechEngine {
    ContentBrowserPanel::ContentBrowserPanel() : Panel("ContentBrowserPanel") {

    }

    void ContentBrowserPanel::init() {
        currentPath = ProjectManager::getUserProjectRootPath();
    }

    void ContentBrowserPanel::onUpdate() {
        /*static float panelSize = 200.0f; // Initial width of the left panel
        //ImGui::Splitter(true, 8.0f, panelSizes);
        ImGui::Begin("Project Browser");
        // Add the left-side panel
        ImGui::BeginChild("##LeftSidePanel", ImVec2(panelSize, 0), true);

        // You can add your folder hierarchy here
        if (ImGui::TreeNode("Folder 1")) {
            if (ImGui::Selectable("Subfolder 1")) {
                // Handle folder selection
            }
            if (ImGui::Selectable("Subfolder 2")) {
                // Handle folder selection
            }
            ImGui::TreePop();
        }

        ImGui::EndChild(); // End of left-side panel
        ImGui::SameLine();

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float rightPanelSize = availableWidth - panelSize;
        ImGui::BeginChild("##RightSidePanel", ImVec2(rightPanelSize, 0), true);
        menuWindowOpen = false;
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
                        deleteFile(path.filename().string());
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
        ImGui::EndChild(); // End of left-side panel
        // Handle resizing by dragging
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
            panelSize += ImGui::GetIO().MouseDelta.x;
            if (panelSize < 50.0f) {
                panelSize = 50.0f; // Minimum width for the left panel
            }
        }

        ImGui::End();*/

        ImGui::Begin("Project Browser");
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        static float size1 = 150;
        float size2 = wsize.x - size1 - 10;
        Splitter(true, 4.0f, &size1, &size2, 150.0f, 100.0f, -1.0f);
        ImGui::BeginChild("##LeftPanel", ImVec2(size1, -1), false);
        if (ImGui::TreeNode(ProjectManager::getUserProjectRootPath().string().substr(ProjectManager::getUserProjectRootPath().string().find_last_of('\\') + 1).c_str())) {
            RenderSceneHierarchy(ProjectManager::getUserProjectRootPath());
            ImGui::TreePop();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##RightPanel", ImVec2(size2, -1), true);
        ImGui::Text("Right Panel Content");

        ImGui::EndChild();

        ImGui::End(); // End of "Project Browser"
    }

    void ContentBrowserPanel::RenderSceneHierarchy(const std::filesystem::path &directoryPath) {
        for (auto &directoryEntry: std::filesystem::directory_iterator(directoryPath)) {
            if (!directoryEntry.is_directory()) {
                continue;
            }
            const auto &path = directoryEntry.path();
            auto relativePath = std::filesystem::relative(path, ProjectManager::getUserProjectRootPath());
            std::string filenameString = relativePath.filename().string();
            bool hasSubDirs = false;
            for (auto &subDir: std::filesystem::directory_iterator(path)) {
                if (subDir.is_directory()) {
                    hasSubDirs = true;
                    break;
                }
            }
            // Define flags based on whether the node is selected or not
            ImGuiTreeNodeFlags flags = ((currentPath == path) ? ImGuiTreeNodeFlags_Selected : 0) |
                                       ImGuiTreeNodeFlags_OpenOnArrow |
                                       (hasSubDirs ? 0 : ImGuiTreeNodeFlags_Leaf) |
                                       ImGuiTreeNodeFlags_SpanFullWidth |
                                       ImGuiTreeNodeFlags_OpenOnArrow;

            bool open = ImGui::TreeNodeEx(filenameString.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                TE_LOGGER_INFO("Clicked on {0}", path.string());
                currentPath = path;
            }
            if (open) {
                RenderSceneHierarchy(path); // Recursive call to render subdirectories
                ImGui::TreePop();
            }
        }
    }

    void ContentBrowserPanel::openFile(const std::string &filename) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        if (extension == ".scene") {
            std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
            SceneManager::loadScene(filenameWithoutExtension);
        }
    }

    void ContentBrowserPanel::renameFile(const std::string &filename, const std::string &newName) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        std::filesystem::rename(currentPath / filename, currentPath / (newName + extension));
    }

    void ContentBrowserPanel::deleteFile(const std::string &filename) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        if (extension == ".scene") {
            std::string path = currentPath.string() + "\\" + filename;
            if (SceneManager::deleteScene(path)) {
                std::filesystem::remove(path);
            }
        }
    }

    bool ContentBrowserPanel::Splitter(bool split_vertically, float thickness, float *size1, float *size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f) {
        using namespace ImGui;
        ImGuiContext &g = *GImGui;
        ImGuiWindow *window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

}
