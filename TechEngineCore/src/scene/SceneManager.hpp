#pragma once


#include "scene/Scene.hpp"


#include <string>
#include <unordered_map>

#include "core/FilePaths.hpp"
#include "material/MaterialManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "yaml-cpp/node/node.h"
#include "core/CompileProject.hpp"

namespace TechEngine {
    class ProjectManager;

    class SceneManager {
    private:
        Scene scene;
        EventDispatcher& eventDispatcher;
        PhysicsEngine& physicsEngine;
        MaterialManager& materialManager;
        TextureManager& textureManager;
        FilePaths& filePaths;
        std::unordered_map<std::string, std::string> m_scenesBank;

        std::string m_activeSceneName;

    public:
        SceneManager(EventDispatcher& eventDispatcher,
                     PhysicsEngine& physicsEngine,
                     MaterialManager& materialManager,
                     TextureManager& textureManager,
                     FilePaths& filePaths);

        void init(const std::vector<std::string>& scenes);

        void registerScene(std::string& scenePath);

        void createNewScene(std::string& scenePath);

        bool deleteScene(std::string& scenePath);

        bool hasScene(const std::string& sceneName);

        void loadScene(const std::string& sceneName);

        void saveScene(const std::string& sceneName);

        void saveCurrentScene();

        void saveSceneAsTemporarily(const std::string& cachPath, CompileProject compileProject);

        void loadSceneFromTemporarily(const std::string& cachPath, CompileProject compileProject);

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
