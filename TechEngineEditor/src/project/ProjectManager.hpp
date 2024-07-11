#pragma once
#include <system/System.hpp>
#include "core/Server.hpp"
#include "core/Client.hpp"
#include <filesystem>


namespace TechEngine {
    enum class CompileMode;

    class ProjectManager : public System {
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

        std::filesystem::path projectGameExportPath;
        std::filesystem::path projectServerExportPath;

        std::filesystem::path cmakeBuildPath;
        std::filesystem::path cmakeListPath;

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

        const std::filesystem::path& getProjectCachePath();

        const std::filesystem::path& getProjectGameExportPath();

        const std::filesystem::path& getProjectServerExportPath();

        const std::filesystem::path& getCmakeBuildPath();

        const std::filesystem::path& getCmakePath();

        const std::filesystem::path& getCmakeListPath();

        const std::filesystem::path& getTechEngineCoreClientLibPath();

        const std::filesystem::path& getTechEngineCoreServerLibPath();

        const std::filesystem::path& getTechEngineClientLibPath();

        const std::filesystem::path& getTechEngineServerLibPath();

        const std::string& getProjectName();

        void createNewProject(const char* string);

        void saveProject();

        void exportProject(ProjectType projectType, CompileMode compileMode);

        void setupPaths();

        void loadEditorProject(const std::string& projectPath);
    };
}
