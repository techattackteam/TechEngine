#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "assimp/AssimpLoader.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    class PanelsManager;

    class ContentBrowserPanel : public Panel {
    private:
        std::filesystem::path currentPath;
        std::filesystem::path selectedPath;

        PanelsManager &panelsManager;
        ProjectManager &projectManager;
        SceneManager &sceneManager;
        MaterialManager &materialManager;
        AssimpLoader assimpLoader;

    public:
        ContentBrowserPanel(PanelsManager &panelsManager, ProjectManager &projectManager, SceneManager &sceneManager, MaterialManager &materialManager);

        void init();

        void onUpdate() override;

        void openFolderOrFile(const std::string &filename);

        void renameFile(const std::string &filename, const std::string &newName);

        void tryLoadModel(const std::string &filepath);

        void deleteFile(const std::string &filename);

        void renderDirectoryHierarchy(const std::filesystem::path &directoryPath);

        void renderDirectoryHierarchyRecurse(const std::filesystem::path &path);

        void renderFiles(const std::filesystem::path &directoryPath);

        bool openMenuPopupItem(const std::filesystem::path &path);

        bool openMenuPopupWindow(const std::filesystem::path &path);
    };
}
