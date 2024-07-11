#pragma once

#include "system/SystemsRegistry.hpp"
#include <filesystem>

namespace TechEngine {
    enum class CORE_DLL ProjectType {
        Editor,
        Client,
        Server
    };

    class CORE_DLL Project {
    private:
        SystemsRegistry& systemsRegistry;

        std::filesystem::path resourcesPath;
        std::filesystem::path assetsPath;
        std::filesystem::path commonResourcesPath;
        std::filesystem::path commonAssetsPath;
        std::filesystem::path cachePath;

        std::filesystem::path projectLocation;
        std::filesystem::path projectFile;
        std::string projectName;

        std::filesystem::path userScriptsDebugDLLPath;
        std::filesystem::path userScriptsReleaseDLLPath;
        std::filesystem::path userScriptsReleaseDebugDLLPath;

    public:
        std::string lastLoadedScene;

        explicit Project(SystemsRegistry& systemsRegistry);


        void loadProject(const std::string& projectLocation, ProjectType projectType);

        void setupPaths(const std::string& projectLocation, ProjectType projectType);

        void setupSettings();

        std::string getProjectName() const;

        const std::filesystem::path& getResourcesPath() const;

        const std::filesystem::path& getCommonResourcesPath() const;

        const std::filesystem::path& getAssetsPath() const;

        const std::filesystem::path& getCommonAssetsPath() const;

        const std::filesystem::path& getUserScriptsDLLPath();
    };
}
