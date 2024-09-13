#pragma once

#include "Scene.hpp"
#include "systems/System.hpp"

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

    public:
        explicit ScenesManager(SystemsRegistry& systemsRegistry);


        void init(AppType appType);

        void registerScene(const std::filesystem::path& scenePath);

        Scene& getActiveScene();

    private:
        void createDefaultScene();
    };
}
