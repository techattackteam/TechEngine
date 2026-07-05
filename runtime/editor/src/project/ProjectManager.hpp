#pragma once
#include "systems/System.hpp"
#include "Project.hpp"
#include "resources/loaders/TextureLoader.hpp"

namespace TechEngine {
    enum class CompileMode;
    class SceneLoader;

    enum class ProjectType {
        Client,
        Server,
        Editor
    };

    class ProjectManager : public System {
    private:
        Project m_clientProject;
        Project m_serverProject;

        SystemsRegistry& m_editorSystemsRegistry;
        SystemsRegistry& m_clientSystemsRegistry;
        SystemsRegistry& m_serverSystemsRegistry;

        std::filesystem::path m_cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\"";;
        std::filesystem::path m_cmakeListPath;
        std::filesystem::path m_techEngineClientLibPath;
        std::filesystem::path m_techEngineServerLibPath;


        std::filesystem::path m_projectPath;
        std::filesystem::path m_projectName;

        std::filesystem::path m_assetsPath;
        std::filesystem::path m_resourcesPath;
        std::filesystem::path m_cachePath;

        SceneLoader* m_sceneLoader = nullptr;

        // Prefer composing paths using filesystem::path operator/ instead of manual string concatenation
        const std::filesystem::path m_runtimesPath = std::filesystem::current_path() / "runtimes";
        const std::filesystem::path m_clientRuntimePath = m_runtimesPath / "client";
        const std::filesystem::path m_serverRuntimesPath = m_runtimesPath / "server";

    public:
        ProjectManager(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& clientSystemsRegistry, SystemsRegistry& serverSystemsRegistry);

        void init(const std::filesystem::path& projectPath, TextureLoader& textureLoader, MaterialLoader& materialLoader, MeshLoader& meshLoader, ModelLoader& modelLoader, SceneLoader& sceneLoader, FileSystem& fileSystem);

        void shutdown() override;

        void createProject(const std::string& projectName);

        void exportProject(const std::filesystem::path& path, ProjectType type);

        void saveProject();

        std::filesystem::path getCmakeListPath() const;

        std::filesystem::path getCmakeBuildPath(CompileMode compileMode);

        std::filesystem::path getCmakePath() const;

        std::filesystem::path getProjectPath() const;

        std::filesystem::path getAssetsPath() const;

        std::filesystem::path getResourcesPath() const;

        std::filesystem::path getCachePath() const;

        std::string getProjectName() const;

    private:
        void createDefaultProject();

        void loadProject(const TextureLoader& textureLoader,
                         const MaterialLoader& materialLoader,
                         const MeshLoader& meshLoader,
                         const ModelLoader& modelLoader,
                         SceneLoader& sceneLoader,
                         FileSystem& fileSystem);

        void loadResources(const TextureLoader& textureLoader, const MaterialLoader& materialLoader, const MeshLoader& meshLoader, const ModelLoader& modelLoader, SceneLoader& sceneLoader) const;
    };
}
