#pragma once
#include "systems/System.hpp"
#include <filesystem>

namespace TechEngine {
    class CORE_DLL ProjectManager : public System {
    private:
        std::filesystem::path m_projectPath;
        std::filesystem::path m_projectName;

    public:
        explicit ProjectManager(const std::filesystem::path& projectPath);

        void init() override;

        void shutdown() override;

        void exportProject(const std::filesystem::path& path);

    private:
        void createDefaultProject();

        void loadProject();
    };
}
