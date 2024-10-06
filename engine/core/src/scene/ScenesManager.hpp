#pragma once

#include "Scene.hpp"
#include "SceneSerializer.hpp"

#include <string>
#include <filesystem>
#include <unordered_map>


namespace TechEngine {
    enum class AppType;
}

namespace TechEngine {
    class CORE_DLL ScenesManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::unordered_map<std::string, std::filesystem::path> m_scenesBank;
        Scene m_activeScene;
        SceneSerializer m_sceneSerializer;

    public:
        explicit ScenesManager(SystemsRegistry& systemsRegistry);

        void init(AppType appType);

        void shutdown() override;

        void createScene(const std::string& name, const std::filesystem::path& path);

        void registerScene(const std::string& name, const std::filesystem::path& scenePath);

        void saveScene();

        void saveScene(const std::filesystem::path& path);

        void loadScene(const std::string& string);

        Scene& getActiveScene();

        void copyScene(const Scene& scene, std::filesystem::path path);
    };
}
