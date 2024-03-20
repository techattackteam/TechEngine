#pragma once

#include <filesystem>
#include "scene/SceneManager.hpp"

using std::filesystem::path;
namespace TechEngine {
    class ProjectManager {
    private:
        SceneManager &sceneManager;
        TextureManager &textureManager;
        MaterialManager &materialManager;

        std::string projectName = "New Project";

        //Editor root path
        path cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\""; //TODO: Make this path relative

        path projectFilePath;
        path projectLocation = std::filesystem::current_path().string() + "\\" + projectName;
        path projectAssetsPath = projectLocation.string() + "\\Assets";
        path projectResourcesPath = projectLocation.string() + "\\Resources";
        path projectExportPath = projectLocation.string() + "\\Build";

        path scriptsBuildPath = projectResourcesPath.string() + "\\cmake\\cmake-build-debug";
        path userScriptsDLLPath = projectResourcesPath.string() + "\\scripts\\build\\debug\\UserScripts.dll";
        path cmakeListPath = projectResourcesPath.string() + "\\cmake";


    public:
        ProjectManager(SceneManager &sceneManager, TextureManager &textureManager, MaterialManager &materialManager);

        void init();

        const path &getProjectFilePath();

        const path &getProjectLocation();

        const path &getProjectAssetsPath();

        const path &getProjectResourcesPath();

        const path &getScriptsDLLPath();

        const path &getScriptsBuildPath();

        const path &getProjectExportPath();

        const path &getCmakePath();

        const path &getCmakeListPath();

        const std::string &getProjectName();

        void createNewProject(const char *string);

        void saveProject();

        void loadProject(const std::string& projectPath);
    };
}
