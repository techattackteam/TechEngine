#include "ProjectManager.hpp"
#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"

#include "files/FileUtils.hpp"
#include "fileSystem/FileSystem.hpp"
#include "Project.hpp"
#include "resources/loaders/MaterialLoader.hpp"
#include "resources/loaders/TextureLoader.hpp"
#include "resources/loaders/MeshLoader.hpp"
#include "resources/loaders/ModelLoader.hpp"
#include "resources/loaders/SceneLoader.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/Widget.hpp"
#include "ui/WidgetsRegistry.hpp"

#include <fstream>

namespace TechEngine {
    ProjectManager::ProjectManager(SystemsRegistry& editorSystemsRegistry,
                                   SystemsRegistry& clientSystemsRegistry,
                                   SystemsRegistry& serverSystemsRegistry) : m_editorSystemsRegistry(editorSystemsRegistry),
                                                                             m_clientSystemsRegistry(clientSystemsRegistry),
                                                                             m_serverSystemsRegistry(serverSystemsRegistry) {
    }

    void ProjectManager::init(const std::filesystem::path& projectPath,
                              TextureLoader& textureLoader,
                              MaterialLoader& materialLoader,
                              MeshLoader& meshLoader,
                              ModelLoader& modelLoader,
                              SceneLoader& sceneLoader,
                              FileSystem& fileSystem) {
        m_sceneLoader = &sceneLoader;
        m_projectPath = projectPath;
        m_clientProject.init(m_projectPath);
        m_serverProject.init(m_projectPath);
        if (!std::filesystem::exists(m_projectPath)) {
            TE_LOGGER_ERROR("Project not found: " + m_projectPath.string());
            createDefaultProject();
        }
        for (auto& entry: std::filesystem::directory_iterator(m_projectPath)) {
            if (entry.path().extension() == ".teproj") {
                m_projectName = entry.path().stem();
                break;
            }
        }
        loadProject(textureLoader, materialLoader, meshLoader, modelLoader, sceneLoader, fileSystem);
    }

    void ProjectManager::shutdown() {
    }

    void ProjectManager::createProject(const std::string& projectName) {
        assert(!projectName.empty());
        std::filesystem::create_directory(m_projectPath);
        std::filesystem::create_directory(m_projectPath.string() + "\\assets");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache");
        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\common");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\common");

        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\client");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\client");

        std::filesystem::create_directory(m_projectPath.string() + "\\assets\\server");
        std::filesystem::create_directory(m_projectPath.string() + "\\resources\\server");
        std::filesystem::create_directory(m_projectPath.string() + "\\cache\\server");

        TE_LOGGER_INFO("New project created at: " + m_projectPath.string());
        std::filesystem::copy(std::filesystem::current_path().string() + "\\resources\\templates\\project",
                              m_projectPath.string(), std::filesystem::copy_options::recursive);
        std::ofstream fout(m_projectPath.string() + "\\" + projectName + ".teproj");
        YAML::Node config;
        config["Project Name"] = projectName;

        YAML::Node client;
        client["Last Loaded Scene"] = "DefaultScene";
        config["Client"] = client;

        YAML::Node server;
        server["Last Loaded Scene"] = "DefaultScene";
        config["Server"] = server;

        fout << config;
        fout.close();
    }

    void ProjectManager::exportProject(const std::filesystem::path& path, ProjectType projectType) {
        assert(!path.empty() && !m_projectName.empty() && !m_projectPath.empty());
        std::string exportPath = path.string() + "\\" + m_projectName.string();
        if (!std::filesystem::exists(exportPath)) {
            std::filesystem::create_directories(exportPath);
        } else {
            for (const auto& entry: std::filesystem::directory_iterator(exportPath)) {
                std::filesystem::remove_all(entry.path());
            }
        }
        std::filesystem::create_directory(exportPath + "\\assets");
        std::filesystem::create_directory(exportPath + "\\assets\\common");
        std::filesystem::create_directory(exportPath + "\\resources");
        std::filesystem::create_directory(exportPath + "\\resources\\common");
        std::filesystem::create_directory(exportPath + "\\cache");
        std::filesystem::create_directory(exportPath + "\\cache\\common");
        FileUtils::copyRecursive(m_projectPath.string() + "\\assets\\common", exportPath + "\\assets\\common",
                                 {".dll", ".exe", ".cpp", ".hpp"}, {});
        FileUtils::copyRecursive(m_projectPath.string() + "\\resources\\common", exportPath + "\\resources\\common", {},
                                 {"TechEngineAPI"});
        FileUtils::copyRecursive(m_projectPath.string() + "\\cache\\common", exportPath + "\\cache\\common", {},
                                 {"findCmake", "libs"});

        if (projectType == ProjectType::Client) {
            Project& project = m_clientSystemsRegistry.getSystem<Project>();
            project.saveProject();
            std::filesystem::create_directory(exportPath + "\\assets\\client");
            std::filesystem::create_directory(exportPath + "\\resources\\client");
            std::filesystem::create_directory(exportPath + "\\cache\\client");
            FileUtils::copyRecursive(m_projectPath.string() + "\\assets\\client", exportPath + "\\assets\\client",
                                     {".dll", ".exe", ".cpp", ".hpp"}, {});
            FileUtils::copyRecursive(m_projectPath.string() + "\\resources\\client", exportPath + "\\resources\\client",
                                     {}, {"TechEngineAPI"});
            FileUtils::copyRecursive(m_projectPath.string() + "\\cache\\client", exportPath + "\\cache\\client", {},
                                     {});
            //Copy runtime files to project folder
            std::filesystem::copy(m_clientRuntimePath, exportPath, std::filesystem::copy_options::recursive);
            std::ofstream fout = std::ofstream(exportPath + "\\" + m_projectName.string() + ".teproj");
            YAML::Node config;
            config[project.projectConfigToString(ProjectConfig::ProjectName)] = m_projectName.string();
            for (const auto& entry: project.getProjectConfigs()) {
                config[project.projectConfigToString(entry.first)] = entry.second;
            }

            fout << config;
            fout.close();
        } else if (projectType == ProjectType::Server) {
            Project& project = m_serverSystemsRegistry.getSystem<Project>();
            project.saveProject();
            std::filesystem::create_directory(exportPath + "\\assets\\server");
            std::filesystem::create_directory(exportPath + "\\resources\\server");
            std::filesystem::create_directory(exportPath + "\\cache\\server");
            std::filesystem::copy(m_projectPath.string() + "\\assets\\server", exportPath + "\\assets\\server",
                                  std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\resources\\server", exportPath + "\\resources\\server",
                                  std::filesystem::copy_options::recursive);
            std::filesystem::copy(m_projectPath.string() + "\\cache\\server", exportPath + "\\cache\\server",
                                  std::filesystem::copy_options::recursive);

            //Copy runtime files to project folder
            std::filesystem::copy(m_serverRuntimesPath, exportPath, std::filesystem::copy_options::recursive);
            std::ofstream fout = std::ofstream(exportPath + "\\" + m_projectName.string() + ".teproj");
            YAML::Node config;
            config[project.projectConfigToString(ProjectConfig::ProjectName)] = m_projectName.string();
            for (const auto& entry: project.getProjectConfigs()) {
                config[project.projectConfigToString(entry.first)] = entry.second;
            }

            fout << config;
            fout.close();
        }
        TE_LOGGER_INFO("Project exported to: " + exportPath);
    }

    void ProjectManager::saveProject() {
        assert(!m_projectName.empty());

        std::ofstream fout(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
        YAML::Node config;
        config["Project Name"] = m_projectName.string();

        YAML::Node client = m_clientProject.saveProject();

        SceneManager& sceneManager = m_clientSystemsRegistry.getSystem<SceneManager>();
        WidgetsSerializer& serializer = m_clientSystemsRegistry.getSystem<WidgetsRegistry>().getSerializer();
        serializer.serializeUI(sceneManager.getActiveSceneName(), m_clientSystemsRegistry);
        if (m_sceneLoader) {
            m_sceneLoader->saveActiveScene();
        }

        config["Client"] = client;


        YAML::Node server = m_serverProject.saveProject();
        config["Server"] = server;

        fout << config;
        fout.close();
    }

    std::filesystem::path ProjectManager::getCmakeListPath() const {
        return m_assetsPath.string();
    }

    std::filesystem::path ProjectManager::getCmakeBuildPath(CompileMode compileMode) {
        assert(compileMode == CompileMode::Debug ||
            compileMode == CompileMode::Release ||
            compileMode == CompileMode::RelWithDebInfo);
        std::string buildMode;
        if (compileMode == CompileMode::Debug) {
            buildMode = "debug";
        } else if (compileMode == CompileMode::Release) {
            buildMode = "release";
        } else if (compileMode == CompileMode::RelWithDebInfo) {
            buildMode = "relwithdebinfo";
        } else {
            TE_LOGGER_ERROR("Unknown compile mode: " + std::to_string(static_cast<int>(compileMode)));
            return "";
        }

        return m_cachePath.string() + "\\common\\cmake-build-" + buildMode;
    }

    std::filesystem::path ProjectManager::getCmakePath() const {
        return m_cmakePath;
    }

    std::filesystem::path ProjectManager::getProjectPath() const {
        return m_projectPath;
    }

    std::filesystem::path ProjectManager::getAssetsPath() const {
        return m_assetsPath;
    }

    std::filesystem::path ProjectManager::getResourcesPath() const {
        return m_resourcesPath;
    }

    std::filesystem::path ProjectManager::getCachePath() const {
        return m_cachePath;
    }

    std::string ProjectManager::getProjectName() const {
        return m_projectName.string();
    }

    void ProjectManager::createDefaultProject() {
        createProject("New Project");
    }

    void ProjectManager::loadProject(const TextureLoader& textureLoader,
                                     const MaterialLoader& materialLoader,
                                     const MeshLoader& meshLoader,
                                     const ModelLoader& modelLoader,
                                     SceneLoader& sceneLoader,
                                     FileSystem& fileSystem) {
        assert(!m_projectName.empty());
        if (!std::filesystem::exists(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj")) {
            TE_LOGGER_ERROR(
                "Project config not found: " + m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
            return;
        }
        YAML::Node config = YAML::LoadFile(m_projectPath.string() + "\\" + m_projectName.string() + ".teproj");
        if (!config["Project Name"].IsDefined()) {
            TE_LOGGER_ERROR("Project Name not found in project config");
            config["Project Name"] = "New Project";
        }
        YAML::Node clientNode = config["Client"];
        m_clientProject.loadProject(m_projectPath, clientNode);

        YAML::Node serverNode = config["Server"];
        m_serverProject.loadProject(m_projectPath, serverNode);

        m_assetsPath = m_projectPath.string() + "\\assets";
        m_resourcesPath = m_projectPath.string() + "\\resources";
        m_cachePath = m_projectPath.string() + "\\cache";
        fileSystem.mount("editorAssetsClient://", getAssetsPath() / "client", 100);
        fileSystem.mount("projectCache://", getCachePath() / "common", 100);

        wchar_t path[FILENAME_MAX] = {0};
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        std::filesystem::path p(path);

        fileSystem.mount("editorAssets://", p.parent_path() / "assets", 100);

        loadResources(textureLoader, materialLoader, meshLoader, modelLoader, sceneLoader);
    }

    void ProjectManager::loadResources(const TextureLoader& textureLoader,
                                       const MaterialLoader& materialLoader,
                                       const MeshLoader& meshLoader,
                                       const ModelLoader& modelLoader,
                                       SceneLoader& sceneLoader) const {
        FileSystem& fileSystem = m_editorSystemsRegistry.getSystem<FileSystem>();
        ResourceSystem& resourceSystem = m_clientSystemsRegistry.getSystem<ResourceSystem>();

        std::vector<std::filesystem::path> files = fileSystem.list("editorAssetsClient://", true);
        std::vector<std::filesystem::path> cacheFiles = fileSystem.list("projectCache://", true);
        files.insert(files.end(), cacheFiles.begin(), cacheFiles.end());

        static const std::string sceneExtension = ".tescene";

        files.erase(std::remove_if(files.begin(), files.end(), [&](const std::filesystem::path& file) {
            const FileStatus status = fileSystem.status(file);
            return status.extension != textureLoader.resourceExtension() &&
                   status.extension != materialLoader.resourceExtension() &&
                   status.extension != meshLoader.resourceExtension() &&
                   status.extension != modelLoader.resourceExtension() &&
                   status.extension != sceneExtension;
        }), files.end());

        auto extensionPriority = [&](const std::filesystem::path& file) {
            const FileStatus status = fileSystem.status(file);
            if (status.extension == textureLoader.resourceExtension()) {
                return 0;
            }
            if (status.extension == materialLoader.resourceExtension()) {
                return 1;
            }
            if (status.extension == meshLoader.resourceExtension()) {
                return 2;
            }
            if (status.extension == sceneExtension) {
                return 3;
            }
            return 1000;
        };

        std::ranges::stable_sort(files, [&](const std::filesystem::path& left, const std::filesystem::path& right) {
            const int leftPriority = extensionPriority(left);
            const int rightPriority = extensionPriority(right);
            if (leftPriority != rightPriority) {
                return leftPriority < rightPriority;
            }
            return left.string() < right.string();
        });

        for (const std::filesystem::path& file: files) {
            FileStatus status = fileSystem.status(file);
            Buffer buffer;
            fileSystem.read(file, buffer);
            if (status.extension == textureLoader.resourceExtension()) {
                textureLoader.deserializeTextureResource(buffer);
            } else if (status.extension == materialLoader.resourceExtension()) {
                materialLoader.deserializeMaterialResource(buffer);
            } else if (status.extension == meshLoader.resourceExtension()) {
                meshLoader.deserializeMeshResource(buffer);
            } else if (status.extension == modelLoader.resourceExtension()) {
                modelLoader.deserializeModelResource(buffer);
            } else if (status.extension == sceneExtension) {
                sceneLoader.loadSceneMetadata(buffer);
            }
        }

        const std::string lastSceneName = m_clientProject.getProjectConfigs().at(ProjectConfig::Scene);
        if (resourceSystem.isSceneRegistered(lastSceneName)) {
            sceneLoader.loadScene(lastSceneName);
        } else {
            TE_LOGGER_ERROR("Last loaded scene not found {0}. Creating default scene", lastSceneName);
            sceneLoader.createScene("DefaultScene", "editorAssetsClient://DefaultScene.tescene");
            sceneLoader.loadScene(std::string("DefaultScene"));
        }
    }
}
