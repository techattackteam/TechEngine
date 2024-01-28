#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "scene/SceneManager.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    ProjectManager::ProjectManager(SceneManager&sceneManager, TextureManager&textureManager, MaterialManager&materialManager) : sceneManager(sceneManager),
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

    const path& ProjectManager::getScriptsDLLPath() {
        return userScriptsDLLPath;
    }

    const path& ProjectManager::getScriptsBuildPath() {
        return scriptsBuildPath;
    }

    const path& ProjectManager::getProjectExportPath() {
        return projectExportPath;
    }

    const path& ProjectManager::getCmakeListPath() {
        return cmakeListPath;
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
        }
        catch (YAML::Exception&e) {
            TE_LOGGER_CRITICAL("Failed to load {0}.teprj file.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
    }

    void ProjectManager::saveProject() {
        sceneManager.saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Project Name" << YAML::Value << projectName;
            out << YAML::Key << "Last scene loaded" << YAML::Value << sceneManager.getActiveSceneName();
            out << YAML::EndMap;
            std::filesystem::path path = projectFilePath;;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream fout(path);
            fout << out.c_str();
        }
        catch (YAML::Exception&e) {
            TE_LOGGER_CRITICAL("Failed to load {0}.teprj file.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
    }

    void ProjectManager::loadProject(const std::string&projectLocation) {
        this->projectLocation = projectLocation;
        this->projectFilePath = this->projectLocation.string() + "\\" + FileSystem::getFileName(projectLocation) + ".teprj";
        projectAssetsPath = this->projectLocation.string() + "\\Assets";
        projectResourcesPath = this->projectLocation.string() + "\\Resources";
        projectExportPath = this->projectLocation.string() + "\\Build";

        scriptsBuildPath = projectResourcesPath.string() + "\\scripts\\build";
        userScriptsDLLPath = scriptsBuildPath.string() + "\\Debug\\UserScripts.dll";
        cmakeListPath = projectResourcesPath.string() + "\\cmake";

        std::string lastSceneLoaded;
        if (std::filesystem::exists(this->projectFilePath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(this->projectFilePath.string());
                projectName = data["Project Name"].as<std::string>();
                lastSceneLoaded = data["Last scene loaded"].as<std::string>();
            }
            catch (YAML::Exception&e) {
                TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFilePath.string(), e.what());
                exit(1);
            }
        }
        else {
            lastSceneLoaded = "DefaultScene";
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Last scene loaded" << YAML::Value << lastSceneLoaded;
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream fout(this->projectFilePath);
            fout << out.c_str();
        }
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".jpg"));
        textureManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".png"));
        materialManager.init(FileSystem::getAllFilesWithExtension(getProjectLocation().string(), ".mat"));
        sceneManager.init(getProjectLocation().string());
        sceneManager.loadScene(lastSceneLoaded);
    }

    const std::string& ProjectManager::getProjectName() {
        return projectName;
    }
}
