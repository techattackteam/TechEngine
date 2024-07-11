#include <fstream>
#include "ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "panels/PanelsManager.hpp"
#include "texture/TextureManager.hpp"

namespace TechEngine {
    ProjectManager::ProjectManager(Client& client, Server& server) : client(client),
                                                                     server(server) {
    }


    void ProjectManager::init() {
        client.systemsRegistry.getSystem<EventDispatcher>().subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
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

    const std::filesystem::path& ProjectManager::getProjectGameExportPath() {
        return projectGameExportPath;
    }

    const std::filesystem::path& ProjectManager::getProjectServerExportPath() {
        return projectServerExportPath;
    }

    const std::filesystem::path& ProjectManager::getCmakeBuildPath() {
        return cmakeBuildPath;
    }

    const std::filesystem::path& ProjectManager::getCmakeListPath() {
        return cmakeListPath;
    }

    const std::filesystem::path& ProjectManager::getTechEngineCoreClientLibPath() {
        return techEngineCoreClientLibPath;
    }

    const std::filesystem::path& ProjectManager::getTechEngineCoreServerLibPath() {
        return techEngineCoreServerLibPath;
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
            out << YAML::Key << "Client last loaded scene" << YAML::Value << "Client";
            out << YAML::Key << "Server last loaded scene" << YAML::Value << "Server";
            out << YAML::EndMap;
            std::ofstream fout(projectFilePath);
            fout << out.c_str();
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load {0}.teprj file.\n      {1}", this->projectFilePath.string(), e.what());
            exit(1);
        }
    }

    void ProjectManager::saveProject() {
        client.systemsRegistry.getSystem<SceneManager>().saveCurrentScene();
        server.systemsRegistry.getSystem<SceneManager>().saveCurrentScene();
        YAML::Emitter out;
        try {
            out << YAML::BeginMap;
            out << YAML::Key << "Client last loaded scene" << YAML::Value << client.systemsRegistry.getSystem<SceneManager>().getActiveSceneName();
            out << YAML::Key << "Server last loaded scene" << YAML::Value << server.systemsRegistry.getSystem<SceneManager>().getActiveSceneName();
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
        this->projectLocation = projectLocation;
        std::string clientLoadedScene;
        std::string serverLoadedScene;
        if (std::filesystem::exists(this->projectFilePath)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(this->projectFilePath.string());
                clientLoadedScene = data["Client last loaded scene"].as<std::string>();
                serverLoadedScene = data["Server last loaded scene"].as<std::string>();
            } catch (YAML::Exception& e) {
                TE_LOGGER_CRITICAL("Failed to load {0} project.\n      {1}", this->projectFilePath.string(), e.what());
                exit(1);
            }
        } else {
            clientLoadedScene = "DefaultScene";
            serverLoadedScene = "DefaultScene";
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Client last loaded scene" << YAML::Value << clientLoadedScene;
            out << YAML::Key << "Server last loaded scene" << YAML::Value << serverLoadedScene;
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream fout(this->projectFilePath);
            fout << out.c_str();
        }
        client.project.lastLoadedScene = clientLoadedScene;
        client.project.setupPaths(projectLocation, ProjectType::Client);
        server.project.lastLoadedScene = serverLoadedScene;
        server.project.setupPaths(projectLocation, ProjectType::Server);
    }

    void ProjectManager::setupPaths() {
        std::vector<std::string> files = FileSystem::getAllFilesWithExtension({projectLocation.string()}, {".teprj"}, false);
        std::string projectFile = files[0];
        this->projectFilePath = projectFile;
        projectName = projectFile.substr(projectFile.find_last_of("\\") + 1, projectFile.find_last_of(".") - projectFile.find_last_of("\\") - 1);
        projectCachePath = this->projectLocation.string() + "\\Cache";
        projectAssetsPath = this->projectLocation.string() + "\\Assets";
        projectGameExportPath = this->projectLocation.string() + "\\Build\\GameBuild";
        projectServerExportPath = this->projectLocation.string() + "\\Build\\ServerBuild";
        std::string scriptsDebugDLLPath = "\\scripts\\build\\debug";
        std::string scriptsReleaseDLLPath = "\\scripts\\build\\release";
        std::string scriptsReleaseDebugDLLPath = "\\scripts\\build\\releaseWithDebug";

        cmakeListPath = projectAssetsPath.string();
        cmakeBuildPath = projectAssetsPath.string() + "\\cmake-build-debug";
#ifdef TE_DEBUG
        techEngineClientLibPath = client.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\debug\\TechEngineClient.lib";
        techEngineServerLibPath = server.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\debug\\TechEngineServer.lib";
        techEngineCoreClientLibPath = client.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\debug\\TechEngineCoreClient.lib";
        techEngineCoreServerLibPath = server.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\debug\\TechEngineCoreServer.lib";
#elif TE_RELEASEDEBUG
        techEngineClientLibPath = client.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineClient.lib";
        techEngineServerLibPath = server.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineServer.lib";
        techEngineCoreClientLibPath = client.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineCoreClient.lib";
        techEngineCoreServerLibPath = server.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineCoreServer.lib";
#elif TE_RELEASE
        techEngineClientLibPath = client.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineClient.lib";
        techEngineServerLibPath = server.project.getResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineServer.lib";
        techEngineCoreClientLibPath = client.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineCoreClient.lib";
        techEngineCoreServerLibPath = server.project.getCommonResourcesPath().string() + "\\TechEngineAPI\\lib\\release\\TechEngineCoreServer.lib";
#endif
    }

    void ProjectManager::exportProject(ProjectType projectType, CompileMode compileMode) {
        SystemsRegistry& systemsRegistry = projectType == ProjectType::Client ? client.systemsRegistry : server.systemsRegistry;
        Project& project = projectType == ProjectType::Client ? client.project : server.project;
        try {
            std::filesystem::path exportPath;
            if (compileMode == CompileMode::RELEASE) { // Happens when building the game to build path. If compileMde is in debug engines is in debug mode
                if (!std::filesystem::exists(projectLocation.string() + "\\Build"))
                    std::filesystem::create_directory(projectLocation.string() + "\\Build");
                exportPath = projectType == ProjectType::Client ? getProjectGameExportPath() : getProjectServerExportPath();
            } else if (compileMode == CompileMode::DEBUG || compileMode == CompileMode::RELEASEDEBUG) { // Happens when editor launches a new process to run the game
                if (!std::filesystem::exists(projectCachePath)) {
                    std::filesystem::create_directory(projectCachePath);
                }
                exportPath = projectCachePath.string() + "\\Client";
            } else {
                TE_LOGGER_ERROR("Export Game: Invalid compile mode");
                return;
            }
            std::filesystem::remove_all(exportPath);
            std::filesystem::create_directory(exportPath);
            std::filesystem::create_directory(exportPath.string() + "\\Assets");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\" + (projectType == ProjectType::Client ? "Client" : "Server"));
            std::filesystem::create_directory(exportPath.string() + "\\Resources");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\" + (projectType == ProjectType::Client ? "Client" : "Server"));
            systemsRegistry.getSystem<SceneManager>().saveCurrentScene();
            std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                        std::filesystem::copy_options::overwrite_existing;

            if (!FileSystem::getAllFilesWithExtension({getProjectAssetsPath().string() + "\\Client"}, {".cpp", ".hpp"}, true).empty() ||
                !FileSystem::getAllFilesWithExtension({getProjectAssetsPath().string() + "\\Common"}, {".cpp", ".hpp"}, true).empty()) {
                // panelsManager.compileClientUserScripts(compileMode);
            }
            FileSystem::copyRecursive(project.getCommonAssetsPath().string(), exportPath.string() + "\\Assets\\Common", {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(project.getAssetsPath().string(), exportPath.string() + "\\Assets\\" + (projectType == ProjectType::Client ? "Client" : "Server"), {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(project.getCommonResourcesPath(), exportPath.string() + "\\Resources\\Common", {".cpp", ".hpp"}, {"dependencies", "TechEngineAPI"});
            FileSystem::copyRecursive(project.getResourcesPath(), exportPath.string() + "\\Resources\\" + (projectType == ProjectType::Client ? "Client" : "Server"), {".cpp", ".hpp"}, {"TechEngineAPI"});

            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Project Name" << YAML::Value << projectName;
            out << YAML::Key << "Last loaded scene" << YAML::Value << project.lastLoadedScene;
            out << YAML::EndMap;

            std::ofstream ofs(exportPath.string() + "\\" + projectName + ".teprj");
            ofs << out.c_str();
            ofs.close();


            std::filesystem::copy(projectType == ProjectType::Client ? FileSystem::clientPath : FileSystem::serverPath, exportPath, copyOptions);
            TE_LOGGER_INFO("Project exported to: {0}", exportPath.string());
        } catch (std::filesystem::filesystem_error& e) {
            TE_LOGGER_ERROR("Error while exporting project: {0}", e.what());
            return;
        }
    }

    const std::string& ProjectManager::getProjectName() {
        return projectName;
    }
}
