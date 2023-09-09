#pragma once

#include <string>
#include <unordered_map>

namespace TechEngine {
    class SceneManager {
    private:
        inline static std::unordered_map<std::string, std::string> m_scenesBank;

        inline static std::string m_activeSceneName;

        static void serialize(const std::string &sceneName, const std::string &filepath);

        static bool deserialize(const std::string &filepath);

        static void replaceSceneNameFromPath(std::string &sceneName);

        static std::string getSceneNameFromPath(const std::string &scenePath);

    public:
        static void init(const std::string &projectPath);

        static void registerScene(std::string &scenePath);

        static void createNewScene(std::string &scenePath);

        static bool deleteScene(std::string &scenePath);

        static void loadScene(const std::string &sceneName);

        static void saveScene(const std::string &sceneName);

        static void saveCurrentScene();

        static void saveSceneAsTemporarily(const std::string &sceneName);

        static void loadSceneFromTemporarily(const std::string &sceneName);

        static const std::string &getActiveSceneName();

    };
}