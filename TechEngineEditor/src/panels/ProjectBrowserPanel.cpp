#include <imgui.h>
#include "project/ProjectManager.hpp"
#include "ProjectBrowserPanel.hpp"

namespace TechEngine {
    ProjectBrowserPanel::ProjectBrowserPanel() : Panel("ProjectBrowserPanel") {

    }

    void ProjectBrowserPanel::init() {
        currentPath = ProjectManager::getUserProjectRootPath();
    }

    void ProjectBrowserPanel::onUpdate() {
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
            } else {
                if (ImGui::Button(filenameString.c_str())) {
                }
            }
        }

        ImGui::End();
    }
}
