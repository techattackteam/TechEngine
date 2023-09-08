#pragma once

#include <filesystem>
#include "core/Core.hpp"

using std::filesystem::path;
namespace TechEngine {
    class ProjectManager {
    private:
        inline static ProjectManager *instance;

        ProjectManager();

        //Editor root path
        path cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\"";
        path rootPath = std::filesystem::current_path();
        path projectTemplate = rootPath.string() + "\\projectTemplate";
        path scriptsTemplate = rootPath.string() + "\\scripts";


        path userProjectRootPath = rootPath.string() + "\\New Project";
        path userProjectScenesPath = userProjectRootPath.string() + "\\scenes";
        path userProjectScriptsPath = userProjectRootPath.string() + "\\scripts";
        path userProjectBuildPath = userProjectScriptsPath.string() + "\\build";

        path buildPath = userProjectRootPath.string() + "\\build";
        path buildResourcesPath = buildPath.string() + "\\resources";

        path userScriptsDLLPath = userProjectBuildPath.string() + "\\Debug\\UserScripts.dll";
        path resourcesPath = rootPath.string() + "\\resources";

        path runtimePath = rootPath.string() + "\\runtime";

        path projectSettings = userProjectRootPath.string() + "\\projectSettings.PjSettings";


        static ProjectManager *getInstance();

    public:
        static const void init(path rootPath);

        static const path &getRootPath();

        static const path &getProjectTemplate();

        static const path &getScriptsTemplate();

        static const path &getBuildPath();

        static const path &getUserProjectRootPath();

        static const path &getUserProjectScriptsPath();

        static const path &getUserProjectScenePath();

        static const path &getUserScriptsDLLPath();

        static const path &getRuntimePath();

        static const path &getBuildResourcesPath();

        static const path &getResourcesPath();

        static const path &getUserProjectBuildPath();

        static const path &getCmakePath();

        static void createNewProject(const char *string);

        static void loadProject(std::string projectPath);
    };
}
