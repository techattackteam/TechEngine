#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/Logger.hpp"
#include "scene/SceneManager.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "event/EventDispatcher.hpp"
#include "core/ConstantPaths.hpp"

namespace TechEngine {

    ProjectManager::ProjectManager() {

    }

    ProjectManager *ProjectManager::getInstance() {
        if (instance == nullptr) {
            instance = new ProjectManager();
        }
        return instance;
    }

    void ProjectManager::init(path rootPath) {
        getInstance()->rootPath = rootPath;
        EventDispatcher::getInstance().subscribe(AppCloseRequestEvent::eventType, [](Event *event) {
            saveProject();
        });
    }

    const path &ProjectManager::getRootPath() {
        return getInstance()->rootPath;
    }

    const path &ProjectManager::getUserProjectRootPath() {
        return getInstance()->userProjectRootPath;
    }

    const path &ProjectManager::getUserProjectScriptsPath() {
        return getInstance()->userProjectScriptsPath;
    }

    const path &ProjectManager::getUserProjectScenePath() {
        return getInstance()->userProjectScenesPath;
    }

    const path &ProjectManager::getUserScriptsDLLPath() {
        return getInstance()->userScriptsDLLPath;
    }

    const path &ProjectManager::getRuntimePath() {
        return getInstance()->runtimePath;
    }

    const path &ProjectManager::getBuildPath() {
        return getInstance()->buildPath;
    }

    const path &ProjectManager::getBuildResourcesPath() {
        return getInstance()->buildResourcesPath;
    }

    const path &ProjectManager::getResourcesPath() {
        return getInstance()->resourcesPath;
    }

    const path &ProjectManager::getUserProjectBuildPath() {
        return getInstance()->userProjectBuildPath;
    }

    const path &ProjectManager::getCmakePath() {
        return getInstance()->cmakePath;
    }

    void ProjectManager::createNewProject(const char *projectName) {
        std::filesystem::create_directory(projectName);
        std::filesystem::copy(Paths::projectTemplate, Paths::rootPath.string() + "\\" + projectName, std::filesystem::copy_options::recursive);
    }

    void ProjectManager::saveProject() {
        SceneManager::saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << SceneManager::getActiveSceneName();
            out << YAML::EndMap;
            std::filesystem::path path = getInstance()->projectSettingsPath;;
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
        getInstance()->userProjectRootPath = projectPath;
        getInstance()->userProjectScriptsPath = projectPath + "/scripts";
        getInstance()->userProjectScenesPath = projectPath + "/scenes";
        getInstance()->userProjectBuildPath = projectPath + "/scripts/build";
        getInstance()->userScriptsDLLPath = projectPath + "/scripts/build/Debug/UserScripts.dll";
        getInstance()->projectSettingsPath = projectPath + "/projectSettings.PjSettings";
        SceneManager::init(projectPath);

        std::string lastSceneLoaded;
        if (std::filesystem::exists(getInstance()->projectSettingsPath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(getInstance()->projectSettingsPath.string());
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
            std::ofstream fout(getInstance()->projectSettingsPath.string());
            fout << out.c_str();
        }
        SceneManager::loadScene(lastSceneLoaded);
    }
}
