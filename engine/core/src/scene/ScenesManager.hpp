#pragma once

#include "systems/System.hpp"
#include "Scene.hpp"
#include "SceneSerializer.hpp"
#include "project/ProjectManager.hpp"

#include <string>
#include <filesystem>
#include <unordered_map>

namespace TechEngine {
    enum class AppType;
}

namespace TechEngine {
    class ScenesManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::unordered_map<std::string, std::filesystem::path> m_scenesBank;
        Scene m_activeScene;
        SceneSerializer m_sceneSerializer;

    public:
        explicit ScenesManager(SystemsRegistry& systemsRegistry);

        void init(AppType appType, std::unordered_map<ProjectConfig, std::string>& projectConfigs);

        void shutdown() override;

        void createScene(const std::string& name, const std::filesystem::path& path);

        void registerScene(const std::string& name, const std::filesystem::path& scenePath);

        void saveScene(const std::filesystem::path& path);

        void loadScene(const std::string& string);

        Scene& getActiveScene();
    };
}
