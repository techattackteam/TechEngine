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
        path projectLocation = std::filesystem::current_path().string() + "\\" + projectName;
        path projectAssetsPath = projectLocation.string() + "\\Assets";
        path projectResourcesPath = projectLocation.string() + "\\Resources";
        path projectExportPath = projectLocation.string() + "\\GameBuild";

        path cmakeBuildPath = projectResourcesPath.string() + "\\cmake\\cmake-build-debug";
        path userScriptsDebugDLLPath = projectResourcesPath.string() + "\\scripts\\build\\debug\\UserScripts.dll";
        path userScriptsReleaseDLLPath = projectResourcesPath.string() + "\\scripts\\build\\release\\UserScripts.dll";
        path userScriptsReleaseDebugDLLPath = projectResourcesPath.string() + "\\scripts\\build\\releaseWithDebug\\UserScripts.dll";
        path cmakeListPath = projectResourcesPath.string() + "\\cmake";
#ifdef TE_DEBUG
        path techEngineCoreLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\debug\\TechEngineCore.lib";
#elif TE_RELEASEDEBUG
        path techEngineCoreLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\releaseWithDebug\\TechEngineCore.lib";
#elif TE_RELEASE
        path tehcengineCoreLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\release\\TechEngineCore.lib";
#endif
#ifdef TE_DEBUG
        path techEngineClientLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\debug\\TechEngineClient.lib";
#elif TE_RELEASEDEBUG
        path techEngineClientLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\releaseWithDebug\\TechEngineClient.lib";
#elif TE_RELEASE
        path techEngineClientLibPath = projectResourcesPath.string() + "\\TechEngineAPI\\lib\\release\\TechEngineClient.lib";
#endif

    public:
        ProjectManager(SceneManager& sceneManager, TextureManager& textureManager, MaterialManager& materialManager);

        void init();

        const path& getProjectFilePath();

        const path& getProjectLocation();

        const path& getProjectAssetsPath();

        const path& getProjectResourcesPath();

        const path& getScriptsDebugDLLPath();

        const path& getScriptsReleaseDLLPath();

        const path& getScriptsReleaseDebugDLLPath();

        const path& getUserScriptsDLLPath();

        const path& getCmakeBuildPath();

        const path& getProjectExportPath();

        const path& getCmakePath();

        const path& getCmakeListPath();

        const path& getTechEngineCoreLibPath();

        const path& getTechEngineClientLibPath();

        const std::string& getProjectName();

        void createNewProject(const char* string);

        void saveProject();

        void loadEditorProject(const std::string& projectPath);

        void loadRuntimeProject(const std::string& projectPath);

    private:
        void setupPaths(const std::string& projectPath);
    };
}
