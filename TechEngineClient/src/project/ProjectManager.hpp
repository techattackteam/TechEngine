#pragma once

#include <filesystem>
#include "scene/SceneManager.hpp"

using std::filesystem::path;
namespace TechEngine {
    class ProjectManager {
    private:
        SceneManager &sceneManager;

        //Editor root path
        path cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\"";
        path rootPath = std::filesystem::current_path();

        path userProjectRootPath = rootPath.string() + "\\New Project";
        path userProjectScenesPath = userProjectRootPath.string() + "\\scenes";
        path userProjectScriptsPath = userProjectRootPath.string() + "\\scripts";
        path userProjectBuildPath = userProjectScriptsPath.string() + "\\build";

        path buildPath = userProjectRootPath.string() + "\\build";
        path buildResourcesPath = buildPath.string() + "\\resources";

        path userScriptsDLLPath = userProjectBuildPath.string() + "\\Debug\\UserScripts.dll";
        path resourcesPath = rootPath.string() + "\\resources";

        path runtimePath = rootPath.string() + "\\runtime";

        path projectSettingsPath = userProjectRootPath.string() + "\\projectSettings.PjSettings";

    public:
        ProjectManager(SceneManager &sceneManager);

        void init(path rootPath);

        const path &getRootPath();

        const path &getProjectTemplate();

        const path &getScriptsTemplate();

        const path &getBuildPath();

        const path &getUserProjectRootPath();

        const path &getUserProjectScriptsPath();

        const path &getUserProjectScenePath();

        const path &getUserScriptsDLLPath();

        const path &getRuntimePath();

        const path &getBuildResourcesPath();

        const path &getResourcesPath();

        const path &getUserProjectBuildPath();

        const path &getCmakePath();

        void createNewProject(const char *string);

        void saveProject();

        void loadProject(std::string projectPath);
    };
}
