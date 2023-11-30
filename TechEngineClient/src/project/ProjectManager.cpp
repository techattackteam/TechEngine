#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "scene/SceneManager.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {

    ProjectManager::ProjectManager(SceneManager &sceneManager) : sceneManager(sceneManager) {

    }


    void ProjectManager::init(path rootPath) {
        rootPath = rootPath;
        EventDispatcher::getInstance().subscribe(AppCloseRequestEvent::eventType, [this](Event *event) {
            saveProject();
        });
    }

    const path &ProjectManager::getRootPath() {
        return rootPath;
    }

    const path &ProjectManager::getUserProjectRootPath() {
        return userProjectRootPath;
    }

    const path &ProjectManager::getUserProjectScriptsPath() {
        return userProjectScriptsPath;
    }

    const path &ProjectManager::getUserProjectScenePath() {
        return userProjectScenesPath;
    }

    const path &ProjectManager::getUserScriptsDLLPath() {
        return userScriptsDLLPath;
    }

    const path &ProjectManager::getRuntimePath() {
        return runtimePath;
    }

    const path &ProjectManager::getBuildPath() {
        return buildPath;
    }

    const path &ProjectManager::getBuildResourcesPath() {
        return buildResourcesPath;
    }

    const path &ProjectManager::getResourcesPath() {
        return resourcesPath;
    }

    const path &ProjectManager::getUserProjectBuildPath() {
        return userProjectBuildPath;
    }

    const path &ProjectManager::getCmakePath() {
        return cmakePath;
    }

    void ProjectManager::createNewProject(const char *projectName) {
        std::filesystem::create_directory(projectName);
        std::filesystem::copy(FileSystem::projectTemplate, FileSystem::rootPath.string() + "\\" + projectName, std::filesystem::copy_options::recursive);
    }

    void ProjectManager::saveProject() {
        sceneManager.saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << sceneManager.getActiveSceneName();
            out << YAML::EndMap;
            std::filesystem::path path = projectSettingsPath;;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream fout(path);
            fout << out.c_str();
        }
        catch (YAML::Exception &e) {
            TE_LOGGER_CRITICAL("Failed to save projectSettings.PjSettings file.\n      {0}", e.what());
            exit(1);
        }
    }

    void ProjectManager::loadProject(std::string projectPath) {
        userProjectRootPath = projectPath;
        userProjectScriptsPath = projectPath + "/scripts";
        userProjectScenesPath = projectPath + "/scenes";
        userProjectBuildPath = projectPath + "/scripts/build";
        userScriptsDLLPath = projectPath + "/scripts/build/Debug/UserScripts.dll";
        projectSettingsPath = projectPath + "/projectSettings.PjSettings";
        sceneManager.init(projectPath);

        std::string lastSceneLoaded;
        if (std::filesystem::exists(projectSettingsPath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(projectSettingsPath.string());
                lastSceneLoaded = data["Last scene loaded"].as<std::string>();
            }
            catch (YAML::Exception &e) {
                TE_LOGGER_CRITICAL("Failed to load EditorSettings.TESettings file.\n      {0}", e.what());
                exit(1);
            }
        } else {
            lastSceneLoaded = "DefaultScene";
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << lastSceneLoaded;
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream fout(projectSettingsPath.string());
            fout << out.c_str();
        }
        sceneManager.loadScene(lastSceneLoaded);
    }
}
