#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Key.hpp"
#include "core/Logger.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    ProjectManager::ProjectManager(Client& client, Server& server) : client(client),
                                                                     server(server) {
    }


    void ProjectManager::init() {
        EventDispatcher::getInstance().subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
            saveProject();
        });
    }

    const std::filesystem::path& ProjectManager::getProjectFilePath() {
        return projectFilePath;
    }

    const std::filesystem::path& ProjectManager::getProjectLocation() {
        return projectLocation;
    }

    const std::filesystem::path& ProjectManager::getProjectAssetsPath() {
        return projectAssetsPath;
    }

    const std::filesystem::path& ProjectManager::getProjectCachePath() {
        return projectCachePath;
    }

    const std::filesystem::path& ProjectManager::getProjectServerResourcesPath() {
        return projectServerResourcesPath;
    }

    const std::filesystem::path& ProjectManager::getProjectCommonResourcesPath() {
        return projectCommonResourcesPath;
    }

    const std::filesystem::path& ProjectManager::getProjectClientResourcesPath() {
        return projectClientResourcesPath;
    }

    const std::filesystem::path& ProjectManager::getClientScriptsDebugDLLPath() {
        return clientUserScriptsDebugDLLPath;
    }

    const std::filesystem::path& ProjectManager::getClientScriptsReleaseDLLPath() {
        return clientUserScriptsReleaseDLLPath;
    }

    const std::filesystem::path& ProjectManager::getClientScriptsReleaseDebugDLLPath() {
        return clientUserScriptsReleaseDebugDLLPath;
    }

    const std::filesystem::path& ProjectManager::getClientUserScriptsDLLPath() {
#ifdef TE_DEBUG
        return clientUserScriptsDebugDLLPath;
#elif TE_RELEASEDEBUG
        return clientUserScriptsReleaseDebugDLLPath;
#elif TE_RELEASE
        return clientUserScriptsReleaseDLLPath;
#endif
    }

    const std::filesystem::path& ProjectManager::getClientCmakeBuildPath() {
        return clientCmakeBuildPath;
    }

    const std::filesystem::path& ProjectManager::getServerScriptsDebugDLLPath() {
        return serverUserScriptsDebugDLLPath;
    }

    const std::filesystem::path& ProjectManager::getServerScriptsReleaseDLLPath() {
        return serverUserScriptsReleaseDLLPath;
    }

    const std::filesystem::path& ProjectManager::getServerScriptsReleaseDebugDLLPath() {
        return serverUserScriptsReleaseDebugDLLPath;
    }

    const std::filesystem::path& ProjectManager::getServerCmakeBuildPath() {
        return serverCmakeBuildPath;
    }

    const std::filesystem::path& ProjectManager::getProjectGameExportPath() {
        return projectGameExportPath;
    }

    const std::filesystem::path& ProjectManager::getProjectServerExportPath() {
        return projectServerExportPath;
    }

    const std::filesystem::path& ProjectManager::getClientCmakeListPath() {
        return clientCmakeListPath;
    }

    const std::filesystem::path& ProjectManager::getServerCmakeListPath() {
        return serverCmakeListPath;
    }

    const std::filesystem::path& ProjectManager::getTechEngineCoreLibPath() {
        return techEngineCoreLibPath;
    }

    const std::filesystem::path& ProjectManager::getTechEngineClientLibPath() {
        return techEngineClientLibPath;
    }

    const std::filesystem::path& ProjectManager::getTechEngineServerLibPath() {
        return techEngineServerLibPath;
    }

    const std::filesystem::path& ProjectManager::getCmakePath() {
        return cmakePath;
    }

    void ProjectManager::createNewProject(const char* projectName) {
        std::filesystem::create_directory(projectName);
        std::filesystem::copy(FileSystem::projectTemplate, FileSystem::rootPath.string() + "\\" + projectName, std::filesystem::copy_options::recursive);
        projectFilePath = FileSystem::rootPath.string() + "\\" + projectName + "\\" + projectName + ".teprj";
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Project Name" << YAML::Value << projectName;
            out << YAML::Key << "Last scene loaded" << YAML::Value << "DefaultScene";
            out << YAML::EndMap;
            std::ofstream fout(projectFilePath);
            fout << out.c_str();
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load {0}.teprj file.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
    }

    void ProjectManager::saveProject() {
        client.sceneManager.saveCurrentScene();
        server.sceneManager.saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Client last loaded scene" << YAML::Value << client.sceneManager.getActiveSceneName();
            out << YAML::Key << "Server last loaded scene" << YAML::Value << server.sceneManager.getActiveSceneName();
            out << YAML::EndMap;
            std::filesystem::path path = projectFilePath;;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream fout(path);
            fout << out.c_str();
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load {0}.teprj file.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
    }

    void ProjectManager::loadEditorProject(const std::string& projectLocation) {
        setupPaths(projectLocation);
        client.filePaths.assetsPath = projectAssetsPath.string();
        client.filePaths.resourcesPath = projectClientResourcesPath.string();
        client.filePaths.commonResourcesPath = projectCommonResourcesPath.string();
        client.filePaths.commonAssetsPath = projectCommonResourcesPath.string();
        server.filePaths.assetsPath = projectAssetsPath.string();
        server.filePaths.resourcesPath = projectServerResourcesPath.string();
        server.filePaths.commonResourcesPath = projectCommonResourcesPath.string();
        server.filePaths.commonAssetsPath = projectCommonResourcesPath.string();

        std::string clientLoadedScene;
        std::string serverLoadedScene;
        if (std::filesystem::exists(this->projectFilePath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(this->projectFilePath.string());
                clientLoadedScene = data["Client last loaded scene"].as<std::string>();
                clientLoadedScene = data["Server last loaded scene"].as<std::string>();
            } catch (YAML::Exception& e) {
                TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFilePath.string(), e.what());
                exit(1);
            }
        } else {
            clientLoadedScene = "DefaultScene";
            clientLoadedScene = "DefaultScene";
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Client last loaded scene" << YAML::Value << clientLoadedScene;
            out << YAML::Key << "Server last loaded scene" << YAML::Value << serverLoadedScene;
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream fout(this->projectFilePath);
            fout << out.c_str();
        }
        client.textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".jpg", ".png"}, true));
        client.materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".mat"}, true));
        client.sceneManager.init(getProjectLocation().string());
        client.sceneManager.loadScene(clientLoadedScene);
    }

    /*void ProjectManager::loadRuntimeProject(const std::string& projectLocation) {
        setupPaths(projectLocation);
        YAML::Node data;
        std::string lastSceneLoaded;
        try {
            data = YAML::LoadFile(this->projectFilePath.string());
            lastSceneLoaded = data["Last scene loaded"].as<std::string>();
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
        /*textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".jpg", ".png"}, true));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".mat"}, true));
        sceneManager.init(getProjectLocation().string());
        sceneManager.loadScene(lastSceneLoaded);#1#
    }*/

    void ProjectManager::setupPaths(const std::string& projectLocation) {
        this->projectLocation = projectLocation;
        std::vector<std::string> files = FileSystem::getAllFilesWithExtension(projectLocation, {".teprj"}, false);
        std::string projectFile = files[0];
        this->projectFilePath = projectFile;
        projectName = projectFile.substr(projectFile.find_last_of("\\") + 1, projectFile.find_last_of(".") - projectFile.find_last_of("\\") - 1);
        projectAssetsPath = this->projectLocation.string() + "\\Assets";
        projectCachePath = this->projectLocation.string() + "\\Cache";
        projectClientResourcesPath = this->projectLocation.string() + "\\Resources\\Client";
        projectCommonResourcesPath = this->projectLocation.string() + "\\Resources\\Common";
        projectServerResourcesPath = this->projectLocation.string() + "\\Resources\\Server";
        projectGameExportPath = this->projectLocation.string() + "\\Build\\GameBuild";
        projectServerExportPath = this->projectLocation.string() + "\\Build\\ServerBuild";
        std::string cmakeBuildPath = "\\cmake\\cmake-build-debug";
        std::string cmakeListPath = "\\cmake";
        std::string scriptsDebugDLLPath = "\\scripts\\build\\debug\\UserScripts.dll";
        std::string scriptsReleaseDLLPath = "\\scripts\\build\\release\\UserScripts.dll";
        std::string scriptsReleaseDebugDLLPath = "\\scripts\\build\\releaseWithDebug\\UserScripts.dll";

        clientCmakeBuildPath = projectClientResourcesPath.string() + cmakeBuildPath;
        clientCmakeListPath = projectClientResourcesPath.string() + cmakeListPath;
        clientUserScriptsDebugDLLPath = projectClientResourcesPath.string() + scriptsDebugDLLPath;
        clientUserScriptsReleaseDLLPath = projectClientResourcesPath.string() + scriptsReleaseDLLPath;
        clientUserScriptsReleaseDebugDLLPath = projectClientResourcesPath.string() + scriptsReleaseDebugDLLPath;

        serverCmakeBuildPath = projectServerResourcesPath.string() + cmakeBuildPath;
        serverCmakeListPath = projectServerResourcesPath.string() + cmakeListPath;
        serverUserScriptsDebugDLLPath = projectServerResourcesPath.string() + scriptsDebugDLLPath;
        serverUserScriptsReleaseDLLPath = projectServerResourcesPath.string() + scriptsReleaseDLLPath;
        serverUserScriptsReleaseDebugDLLPath = projectServerResourcesPath.string() + scriptsReleaseDebugDLLPath;
    }

    const std::string& ProjectManager::getProjectName() {
        return projectName;
    }
}
