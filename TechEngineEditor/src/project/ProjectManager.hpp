#pragma once
#include <filesystem>
#include "core/Server.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class ProjectManager {
    private:
        Client& client;
        Server& server;

        std::string projectName = "New Project";

        //Editor root path
        std::filesystem::path cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\""; //TODO: Make this path relative

        std::filesystem::path projectFilePath;
        std::filesystem::path projectLocation;
        std::filesystem::path projectCachePath;
        std::filesystem::path projectAssetsPath;
        std::filesystem::path projectCommonAssetsPath;
        std::filesystem::path projectClientAssetsPath;
        std::filesystem::path projectServerAssetsPath;
        std::filesystem::path projectCommonResourcesPath;
        std::filesystem::path projectClientResourcesPath;
        std::filesystem::path projectServerResourcesPath;

        std::filesystem::path projectGameExportPath;
        std::filesystem::path projectServerExportPath;

        std::filesystem::path clientCmakeBuildPath;
        std::filesystem::path clientUserScriptsDebugDLLPath;
        std::filesystem::path clientUserScriptsReleaseDLLPath;
        std::filesystem::path clientUserScriptsReleaseDebugDLLPath;
        std::filesystem::path clientCmakeListPath;

        std::filesystem::path serverCmakeBuildPath;
        std::filesystem::path serverUserScriptsDebugDLLPath;
        std::filesystem::path serverUserScriptsReleaseDLLPath;
        std::filesystem::path serverUserScriptsReleaseDebugDLLPath;
        std::filesystem::path serverCmakeListPath;

        std::filesystem::path techEngineCoreClientLibPath;
        std::filesystem::path techEngineCoreServerLibPath;
        std::filesystem::path techEngineClientLibPath;
        std::filesystem::path techEngineServerLibPath;

    public:
        ProjectManager(Client& client, Server& server);

        void init();

        const std::filesystem::path& getProjectFilePath();

        const std::filesystem::path& getProjectLocation();

        const std::filesystem::path& getProjectAssetsPath();

        const std::filesystem::path& getProjectCommonAssetsPath();

        const std::filesystem::path& getProjectClientAssetsPath();

        const std::filesystem::path& getProjectServerAssetsPath();

        const std::filesystem::path& getProjectCachePath();

        const std::filesystem::path& getProjectServerResourcesPath();

        const std::filesystem::path& getProjectCommonResourcesPath();

        const std::filesystem::path& getProjectClientResourcesPath();

        const std::filesystem::path& getClientScriptsDebugDLLPath();

        const std::filesystem::path& getClientScriptsReleaseDLLPath();

        const std::filesystem::path& getClientScriptsReleaseDebugDLLPath();

        const std::filesystem::path& getClientUserScriptsDLLPath();

        const std::filesystem::path& getClientCmakeBuildPath();

        const std::filesystem::path& getServerUserScriptsDLLPath();

        const std::filesystem::path& getServerCmakeBuildPath();

        const std::filesystem::path& getProjectGameExportPath();

        const std::filesystem::path& getProjectServerExportPath();

        const std::filesystem::path& getCmakePath();

        const std::filesystem::path& getClientCmakeListPath();

        const std::filesystem::path& getServerCmakeListPath();

        const std::filesystem::path& getTechEngineCoreClientLibPath();

        const std::filesystem::path& getTechEngineCoreServerLibPath();

        const std::filesystem::path& getTechEngineClientLibPath();

        const std::filesystem::path& getTechEngineServerLibPath();

        const std::string& getProjectName();

        void createNewProject(const char* string);

        void saveProject();

        void loadEditorProject(const std::string& projectPath);

    private:
        void setupPaths(const std::string& projectPath);
    };
}
