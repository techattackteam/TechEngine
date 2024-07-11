#include "Project.hpp"

#include "core/FileSystem.hpp"
#include "yaml-cpp/yaml.h"

namespace TechEngine {
    Project::Project(SystemsRegistry& systemsRegistry) : systemsRegistry(systemsRegistry) {
    }

    void Project::loadProject(const std::string& projectLocation, ProjectType projectType) {
        setupPaths(projectLocation, projectType);
        setupSettings();
    }

    void Project::setupPaths(const std::string& projectLocation, ProjectType projectType) {
        this->projectLocation = projectLocation;
        projectFile = FileSystem::getAllFilesWithExtension({projectLocation}, {".teprj"}, true)[0];
        projectName = FileSystem::getFileName(projectFile.string());
        cachePath = this->projectLocation.string() + "\\Cache";
        assetsPath = this->projectLocation.string() + "\\Assets";
        commonAssetsPath = this->projectLocation.string() + "\\Assets\\Common";
        assetsPath = this->projectLocation.string() + "\\Assets\\" + (projectType == ProjectType::Client ? "Client" : "Server");
        commonResourcesPath = this->projectLocation.string() + "\\Resources\\Common";
        resourcesPath = this->projectLocation.string() + "\\Resources\\" + (projectType == ProjectType::Client ? "Client" : "Server");
        std::string scriptsDebugDLLPath = "\\scripts\\build\\debug";
        std::string scriptsReleaseDLLPath = "\\scripts\\build\\release";
        std::string scriptsReleaseDebugDLLPath = "\\scripts\\build\\releaseWithDebug";
        userScriptsDebugDLLPath = resourcesPath.string() + scriptsDebugDLLPath + "\\" + (projectType == ProjectType::Client ? "ClientScripts.dll" : "ServerScripts.dll");
        userScriptsReleaseDLLPath = resourcesPath.string() + scriptsReleaseDLLPath + "\\" + (projectType == ProjectType::Client ? "ClientScripts.dll" : "ServerScripts.dll");
        userScriptsReleaseDebugDLLPath = resourcesPath.string() + scriptsReleaseDebugDLLPath + "\\" + (projectType == ProjectType::Client ? "ClientScripts.dll" : "ServerScripts.dll");
    }

    void Project::setupSettings() {
        YAML::Node data;
        try {
            data = YAML::LoadFile(this->projectFile.string());
            lastLoadedScene = data["Last loaded scene"].as<std::string>();
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFile.string(), e.what());
            exit(1);
        }
    }

    std::string Project::getProjectName() const {
        return projectName;
    }


    const std::filesystem::path& Project::getResourcesPath() const {
        return resourcesPath;
    }

    const std::filesystem::path& Project::getCommonResourcesPath() const {
        return commonResourcesPath;
    }

    const std::filesystem::path& Project::getAssetsPath() const {
        return assetsPath;
    }

    const std::filesystem::path& Project::getCommonAssetsPath() const {
        return commonAssetsPath;
    }

    const std::filesystem::path& Project::getUserScriptsDLLPath() {
#ifdef TE_DEBUG
        return userScriptsDebugDLLPath;
#elif TE_RELEASEDEBUG
        return userScriptsReleaseDebugDLLPath;
#elif TE_RELEASE
        return userScriptsReleaseDLLPath;
#endif
    }
}
