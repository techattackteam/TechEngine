#pragma once

#include <filesystem>
#include "core/Core.hpp"

using std::filesystem::path;

class ProjectManager {
private:
    inline static ProjectManager *instance;

    ProjectManager();

    //Editor root path
    path rootPath = std::filesystem::current_path();

    path buildPath = rootPath.string() + "/build";
    path buildResourcesPath = buildPath.string() + "/resources";

    path userProjectRootPath = rootPath.string() + "/project";
    path userProjectScenesPath = userProjectRootPath.string() + "/scenes";
    path userProjectScriptsPath = userProjectRootPath.string() + "/scripts";
    path userProjectBuildPath = userProjectScriptsPath.string() + "/build";
public:


private:
    path userScriptsDLLPath = userProjectBuildPath.string() + "/Debug/UserScripts.dll";

    path resourcesPath = rootPath.string() + "/resources";

    path runtimePath = rootPath.string() + "/runtime";

    path engineExportSettingsFile = rootPath.string() + "/EngineSettings.TESettings";

    static ProjectManager *getInstance();

public:
    static const path &getRootPath();

    static const path &getBuildPath();

    static const path &getUserProjectRootPath();

    static const path &getUserProjectScriptsPath();

    static const path &getUserProjectScenePath();

    static const path &getUserScriptsDLLPath();

    static const path &getEngineExportSettingsFile();

    static const path &getRuntimePath();

    static const path &getBuildResourcesPath();

    static const path &getResourcesPath();

    static const path &getUserProjectBuildPath();
};
