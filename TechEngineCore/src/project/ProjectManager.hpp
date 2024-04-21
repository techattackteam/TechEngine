#pragma once

#include <filesystem>
#include "scene/SceneManager.hpp"

using std::filesystem::path;

namespace TechEngine {
    class ProjectManager {
    private:
        SceneManager& sceneManager;
        TextureManager& textureManager;
        MaterialManager& materialManager;

        std::string projectName = "New Project";

        //Editor root path
        path cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\""; //TODO: Make this path relative

        path projectFilePath;
        path projectLocation;
        path projectAssetsPath;
        path projectCachePath;
        path projectCommonResourcesPath;
        path projectClientResourcesPath;
        path projectServerResourcesPath;

        path projectGameExportPath;
        path projectServerExportPath;

        path clientCmakeBuildPath;
        path clientUserScriptsDebugDLLPath;
        path clientUserScriptsReleaseDLLPath;
        path clientUserScriptsReleaseDebugDLLPath;
        path clientCmakeListPath;

        path serverCmakeBuildPath;
        path serverUserScriptsDebugDLLPath;
        path serverUserScriptsReleaseDLLPath;
        path serverUserScriptsReleaseDebugDLLPath;
        path serverCmakeListPath;

        path techEngineCoreLibPath;
        path techEngineClientLibPath;
        path techEngineServerLibPath;

    public:
        ProjectManager(SceneManager& sceneManager, TextureManager& textureManager, MaterialManager& materialManager);

        void init();

        const path& getProjectFilePath();

        const path& getProjectLocation();

        const path& getProjectAssetsPath();

        const path& getProjectCachePath();

        const path& getProjectServerResourcesPath();

        const path& getProjectCommonResourcesPath();

        const path& getProjectClientResourcesPath();

        const path& getClientScriptsDebugDLLPath();

        const path& getClientScriptsReleaseDLLPath();

        const path& getClientScriptsReleaseDebugDLLPath();

        const path& getClientUserScriptsDLLPath();

        const path& getClientCmakeBuildPath();

        const path& getServerScriptsDebugDLLPath();

        const path& getServerScriptsReleaseDLLPath();

        const path& getServerScriptsReleaseDebugDLLPath();

        const path& getServerCmakeBuildPath();

        const path& getProjectGameExportPath();

        const path& getProjectServerExportPath();

        const path& getCmakePath();

        const path& getClientCmakeListPath();

        const path& getServerCmakeListPath();

        const path& getTechEngineCoreLibPath();

        const path& getTechEngineClientLibPath();

        const path& getTechEngineServerLibPath();

        const std::string& getProjectName();

        void createNewProject(const char* string);

        void saveProject();

        void loadEditorProject(const std::string& projectPath);

        void loadRuntimeProject(const std::string& projectPath);

    private:
        void setupPaths(const std::string& projectPath);
    };
}
