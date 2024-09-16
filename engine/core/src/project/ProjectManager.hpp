#pragma once
#include "systems/System.hpp"
#include <filesystem>
#include <unordered_map>

namespace TechEngine {
    enum class CompileMode;

    enum class ProjectType {
        Client,
        Server,
        Editor
    };

    enum class ProjectConfig {
        ProjectName,
        ProjectPath,
        ClientScene,
        ServerScene,
    };

    class CORE_DLL ProjectManager : public System {
    private:
        std::filesystem::path m_cmakePath = "\"C:/Program Files/CMake/bin/cmake.exe\"";;
        std::filesystem::path m_cmakeListPath;
        std::filesystem::path m_techEngineClientLibPath;
        std::filesystem::path m_techEngineServerLibPath;

        std::unordered_map<ProjectConfig, std::string> m_projectConfigs;

        std::filesystem::path m_projectPath;
        std::filesystem::path m_projectName;

        std::filesystem::path m_assetsPath;
        std::filesystem::path m_resourcesPath;
        std::filesystem::path m_cachePath;

        const std::filesystem::path m_runtimesPath = std::filesystem::current_path().string() + "\\runtimes";
        const std::filesystem::path m_clientRuntimePath = m_runtimesPath.string() + "\\client";
        const std::filesystem::path m_serverRuntimesPath = m_runtimesPath.string() + "\\server";

    public:
        explicit ProjectManager(const std::filesystem::path& projectPath);

        void init() override;

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

        std::unordered_map<ProjectConfig, std::string>& getProjectConfigs();

    private:
        void createDefaultProject();

        void loadProject();
    };
}
