#pragma once

#include <string>
#include <unordered_map>

#include "events/material/MaterialUpdateEvent.hpp"
#include "physics/PhysicsEngine.hpp"
#include "yaml-cpp/node/node.h"
#include "scene/Scene.hpp"
#include "material/MaterialManager.hpp"


namespace TechEngine {
    class ProjectManager;

    class SceneManager {
    private:
        Scene scene;
        PhysicsEngine& physicsEngine;
        MaterialManager& materialManager;
        TextureManager& textureManager;
        ProjectManager& projectManager;
        std::unordered_map<std::string, std::string> m_scenesBank;

        std::string m_activeSceneName;

        void serialize(const std::string& sceneName, const std::string& filepath);

        void deserializeGameObject(YAML::Node gameObjectYAML, GameObject* parent);

        bool deserialize(const std::string& filepath);

        void replaceSceneNameFromPath(std::string& sceneName);

        std::string getSceneNameFromPath(const std::string& scenePath);

        //TODO: change this to the actual Scene class
        void onMaterialUpdateEvent(MaterialUpdateEvent& event);

    public:
        SceneManager(ProjectManager& projectManager, PhysicsEngine& physicsEngine, MaterialManager& materialManager, TextureManager& textureManager);

        void init(const std::string& projectPath);

        void registerScene(std::string& scenePath);

        void createNewScene(std::string& scenePath);

        bool deleteScene(std::string& scenePath);

        void loadScene(const std::string& sceneName);

        void saveScene(const std::string& sceneName);

        void saveCurrentScene();

        void saveSceneAsTemporarily(const std::string& sceneName);

        void loadSceneFromTemporarily(const std::string& sceneName);

        const std::string& getActiveSceneName();

        Scene& getScene();
    };
}
