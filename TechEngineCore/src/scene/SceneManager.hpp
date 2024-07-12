#pragma once

#include "scene/Scene.hpp"
#include "material/MaterialManager.hpp"
#include "project/Project.hpp"
#include "yaml-cpp/node/node.h"

namespace TechEngine {
    class CORE_DLL SceneManager : public System {
    private:
        Scene scene;
        SystemsRegistry& systemsRegistry;
        std::unordered_map<std::string, std::string> m_scenesBank;

        std::string m_activeSceneName;

    public:
        SceneManager(SystemsRegistry& systemsRegistry);

        void init(const std::vector<std::string>& scenes);

        void registerScene(std::string& scenePath);

        void createNewScene(std::string& scenePath);

        bool deleteScene(std::string& scenePath);

        bool hasScene(const std::string& sceneName);

        void loadScene(const std::string& sceneName);

        void saveScene(const std::string& sceneName);

        void saveCurrentScene();

        void saveSceneAsTemporarily(const std::string& cachPath, ProjectType projectType);

        void loadSceneFromTemporarily(const std::string& cachPath, ProjectType projectType);

        const std::string& getActiveSceneName();

        Scene& getScene();

    private:
        void serialize(const std::string& sceneName, const std::string& filepath);

        void deserializeGameObject(YAML::Node gameObjectYAML, GameObject* parent);

        bool deserialize(const std::string& filepath);

        void replaceSceneNameFromPath(std::string& sceneName);

        std::string getSceneNameFromPath(const std::string& scenePath);

        //TODO: change this to the actual Scene class
        void onMaterialUpdateEvent(MaterialUpdateEvent& event);
    };
}
