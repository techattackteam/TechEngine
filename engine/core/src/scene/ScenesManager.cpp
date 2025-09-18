#include "ScenesManager.hpp"

#include "events/scene/SceneLoadEvent.hpp"
#include "events/scene/SceneSaveEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "files/FileUtils.hpp"
#include "project/Project.hpp"
#include "resources/ResourcesManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "systems/SystemsRegistry.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    ScenesManager::ScenesManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                                     m_sceneSerializer(m_activeScene,
                                                                                       m_systemsRegistry.getSystem<ResourcesManager>(),
                                                                                       m_systemsRegistry.getSystem<PhysicsEngine>()) {
    }


    void ScenesManager::init(AppType appType) {
        std::vector<std::string> paths = {
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, appType).string()
        };
        std::vector<std::string> filesByExtension = FileUtils::getAllFilesWithExtension(paths, {".tescene"}, true);
        for (const std::string& scenePath: filesByExtension) {
            std::string name = m_sceneSerializer.getSceneName(scenePath);
            registerScene(name, scenePath);
        }
        std::string sceneName = m_systemsRegistry.getSystem<Project>().getProjectConfigs().at(ProjectConfig::Scene);
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            loadScene(sceneName);
        } else {
            TE_LOGGER_WARN("Scene not found: {}.\tCreating default one.", sceneName);
            createScene(sceneName, m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, appType) / (sceneName + ".tescene"));
            saveScene(m_scenesBank[sceneName]);
        }
    }

    void ScenesManager::shutdown() {
        m_activeScene.clear();
        m_scenesBank.clear();
    }

    void ScenesManager::createScene(const std::string& name, const std::filesystem::path& path) {
        m_activeScene.setName(name);
        ResourcesManager& resourcesManager = m_systemsRegistry.getSystem<ResourcesManager>();
        Entity mainCamera = m_activeScene.createEntity("Main Camera");
        m_activeScene.addComponent(mainCamera, Camera());
        Entity cube = m_activeScene.createEntity("Cube");
        m_activeScene.addComponent(cube, MeshRenderer(resourcesManager.getDefaultMesh(), resourcesManager.getDefaultMaterial()));
        MeshRenderer& meshRenderer = m_activeScene.getComponent<MeshRenderer>(cube);
        meshRenderer.paintMesh();
        registerScene(name, path);
    }

    Scene& ScenesManager::getActiveScene() {
        return m_activeScene;
    }

    void ScenesManager::copyScene(const Scene& scene, std::filesystem::path path) {
        saveScene(path);
    }

    void ScenesManager::registerScene(const std::string& name, const std::filesystem::path& scenePath) {
        m_scenesBank[name] = scenePath;
    }

    void ScenesManager::saveScene() {
        saveScene(m_scenesBank[m_activeScene.getName()]);
    }

    void ScenesManager::saveScene(const std::filesystem::path& path) {
        std::ofstream stream(path);
        m_sceneSerializer.serialize(stream);
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<SceneSaveEvent>(m_activeScene.getName());
    }

    void ScenesManager::loadScene(const std::string& name) {
        const std::filesystem::path& path = m_scenesBank[name];
        loadScene(path);
    }

    void ScenesManager::loadScene(const std::filesystem::path& path) {
        m_activeScene.clear();
        m_sceneSerializer.deserialize(path);
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<SceneLoadEvent>(m_activeScene.getName());
    }
}
