#pragma once
#include "systems/System.hpp"
#include <filesystem>

namespace TechEngine {
    enum class ProjectType {
        CLIENT,
        SERVER,
        EDITOR
    };

    class CORE_DLL ProjectManager : public System {
    private:
        std::filesystem::path m_projectPath;
        std::filesystem::path m_projectName;
        const std::filesystem::path m_runtimesPath = std::filesystem::current_path().string() + "\\runtimes";
        const std::filesystem::path m_clientRuntimePath = m_runtimesPath.string() + "\\client";
        const std::filesystem::path m_serverRuntimesPath = m_runtimesPath.string() + "\\server";

    public:
        explicit ProjectManager(const std::filesystem::path& projectPath);

        void init() override;

        void shutdown() override;

        void exportProject(const std::filesystem::path& path, ProjectType type);

        void createProject(const std::string& projectName);

    private:
        void createDefaultProject();

        void loadProject();
    };
}
