#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "scene/SceneManager.hpp"
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

    const path& ProjectManager::getProjectResourcesPath() {
        return projectResourcesPath;
    }

    const path& ProjectManager::getScriptsDebugDLLPath() {
        return userScriptsDebugDLLPath;
    }

    const path& ProjectManager::getScriptsReleaseDLLPath() {
        return userScriptsReleaseDLLPath;
    }

    const path& ProjectManager::getScriptsReleaseDebugDLLPath() {
        return userScriptsReleaseDebugDLLPath;
    }

    const path& ProjectManager::getUserScriptsDLLPath() {
#ifdef TE_DEBUG
        return userScriptsDebugDLLPath;
#elif TE_RELEASEDEBUG
        return userScriptsReleaseDebugDLLPath;
#elif TE_RELEASE
        return userScriptsReleaseDLLPath;
#endif
    }

    const path& ProjectManager::getCmakeBuildPath() {
        return cmakeBuildPath;
    }

    const path& ProjectManager::getProjectExportPath() {
        return projectExportPath;
    }

    const path& ProjectManager::getCmakeListPath() {
        return cmakeListPath;
    }

    const path& ProjectManager::getTechEngineCoreLibPath() {
        return techEngineCoreLibPath;
    }

    const path& ProjectManager::getTechEngineClientLibPath() {
        return techEngineClientLibPath;
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
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".jpg", true));
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".png", true));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".mat", true));
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
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".jpg", true));
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".png", true));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".mat", true));
        sceneManager.init(getProjectLocation().string());
        sceneManager.loadScene(lastSceneLoaded);
    }

    void ProjectManager::setupPaths(const std::string& projectLocation) {
        this->projectLocation = projectLocation;
        std::vector<std::string> files = FileSystem::getAllFilesWithExtension(projectLocation, ".teprj", false);
        std::string projectFile = files[0];
        this->projectFilePath = projectFile;
        projectName = projectFile.substr(projectFile.find_last_of("\\") + 1, projectFile.find_last_of(".") - projectFile.find_last_of("\\") - 1);
        projectAssetsPath = this->projectLocation.string() + "\\Assets";
        projectResourcesPath = this->projectLocation.string() + "\\Resources";
        projectExportPath = this->projectLocation.string() + "\\Build";

        cmakeBuildPath = projectResourcesPath.string() + "\\cmake\\cmake-build-debug";
        userScriptsDebugDLLPath = projectResourcesPath.string() + "\\scripts\\build\\debug\\UserScripts.dll";
        userScriptsReleaseDLLPath = projectResourcesPath.string() + "\\scripts\\build\\release\\UserScripts.dll";
        userScriptsReleaseDebugDLLPath = projectResourcesPath.string() + "\\scripts\\build\\releaseWithDebug\\UserScripts.dll";
        cmakeListPath = projectResourcesPath.string() + "\\cmake";
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
    }

    const std::string& ProjectManager::getProjectName() {
        return projectName;
    }
}
