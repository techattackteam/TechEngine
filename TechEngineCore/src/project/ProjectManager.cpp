#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    ProjectManager::ProjectManager(SceneManager& sceneManager, TextureManager& textureManager, MaterialManager& materialManager) : sceneManager(sceneManager),
                                                                                                                                   textureManager(textureManager),
                                                                                                                                   materialManager(materialManager) {
    }


    void ProjectManager::init() {
        EventDispatcher::getInstance().subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
            saveProject();
        });
    }

    const path& ProjectManager::getProjectFilePath() {
        return projectFilePath;
    }

    const path& ProjectManager::getProjectLocation() {
        return projectLocation;
    }

    const path& ProjectManager::getProjectAssetsPath() {
        return projectAssetsPath;
    }

    const path& ProjectManager::getProjectCachePath() {
        return projectCachePath;
    }

    const path& ProjectManager::getProjectServerResourcesPath() {
        return projectServerResourcesPath;
    }

    const path& ProjectManager::getProjectCommonResourcesPath() {
        return projectCommonResourcesPath;
    }

    const path& ProjectManager::getProjectClientResourcesPath() {
        return projectClientResourcesPath;
    }

    const path& ProjectManager::getClientScriptsDebugDLLPath() {
        return clientUserScriptsDebugDLLPath;
    }

    const path& ProjectManager::getClientScriptsReleaseDLLPath() {
        return clientUserScriptsReleaseDLLPath;
    }

    const path& ProjectManager::getClientScriptsReleaseDebugDLLPath() {
        return clientUserScriptsReleaseDebugDLLPath;
    }

    const path& ProjectManager::getClientUserScriptsDLLPath() {
#ifdef TE_DEBUG
        return clientUserScriptsDebugDLLPath;
#elif TE_RELEASEDEBUG
        return clientUserScriptsReleaseDebugDLLPath;
#elif TE_RELEASE
        return clientUserScriptsReleaseDLLPath;
#endif
    }

    const path& ProjectManager::getClientCmakeBuildPath() {
        return clientCmakeBuildPath;
    }

    const path& ProjectManager::getServerScriptsDebugDLLPath() {
        return serverUserScriptsDebugDLLPath;
    }

    const path& ProjectManager::getServerScriptsReleaseDLLPath() {
        return serverUserScriptsReleaseDLLPath;
    }

    const path& ProjectManager::getServerScriptsReleaseDebugDLLPath() {
        return serverUserScriptsReleaseDebugDLLPath;
    }

    const path& ProjectManager::getServerCmakeBuildPath() {
        return serverCmakeBuildPath;
    }

    const path& ProjectManager::getProjectGameExportPath() {
        return projectGameExportPath;
    }

    const path& ProjectManager::getProjectServerExportPath() {
        return projectServerExportPath;
    }

    const path& ProjectManager::getClientCmakeListPath() {
        return clientCmakeListPath;
    }

    const path& ProjectManager::getServerCmakeListPath() {
        return serverCmakeListPath;
    }

    const path& ProjectManager::getTechEngineCoreLibPath() {
        return techEngineCoreLibPath;
    }

    const path& ProjectManager::getTechEngineClientLibPath() {
        return techEngineClientLibPath;
    }

    const path& ProjectManager::getTechEngineServerLibPath() {
        return techEngineServerLibPath;
    }

    const path& ProjectManager::getCmakePath() {
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
        sceneManager.saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << sceneManager.getActiveSceneName();
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
        std::string lastSceneLoaded;
        if (std::filesystem::exists(this->projectFilePath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(this->projectFilePath.string());
                //projectName = data["Project Name"].as<std::string>();
                lastSceneLoaded = data["Last scene loaded"].as<std::string>();
            } catch (YAML::Exception& e) {
                TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFilePath.string(), e.what());
                exit(1);
            }
        } else {
            lastSceneLoaded = "DefaultScene";
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << lastSceneLoaded;
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream fout(this->projectFilePath);
            fout << out.c_str();
        }
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".jpg", ".png"}, true));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".mat"}, true));
        sceneManager.init(getProjectLocation().string());
        sceneManager.loadScene(lastSceneLoaded);
    }

    void ProjectManager::loadRuntimeProject(const std::string& projectLocation) {
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
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".jpg", ".png"}, true));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), {".mat"}, true));
        sceneManager.init(getProjectLocation().string());
        sceneManager.loadScene(lastSceneLoaded);
    }

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
