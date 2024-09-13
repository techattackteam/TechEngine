#include "ScenesManager.hpp"

#include "files/FileUtils.hpp"
#include "files/PathsBank.hpp"
#include "project/ProjectManager.hpp"
#include "resources/ResourcesManager.hpp"
#include "resources/material/MaterialManager.hpp"
#include "resources/mesh/MeshManager.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ScenesManager::ScenesManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void ScenesManager::init(AppType appType) {
        std::vector<std::string> paths = {
            m_systemsRegistry.getSystem<PathsBank>().getPath(PathType::Assets, AppType::Common).string(),
            m_systemsRegistry.getSystem<PathsBank>().getPath(PathType::Assets, appType).string()
        };
        std::vector<std::string> filesByExtension = FileUtils::getAllFilesWithExtension(paths, {".tescene"}, true);
        for (const std::string& scenePath: filesByExtension) {
            registerScene(scenePath);
        }
        /*ProjectManager& projectManager = m_systemsRegistry.getSystem<ProjectManager>();
        std::string sceneName = projectManager.getProjectConfig(appType == AppType::Client ? ProjectConfig::ClientScene : ProjectConfig::ServerScene);
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            //m_activeScene.loadScene(m_scenesBank[sceneName]);
        } else {
        }*/
        createDefaultScene();
    }


    void ScenesManager::registerScene(const std::filesystem::path& scenePath) {
        std::string sceneName = scenePath.stem().string();
        m_scenesBank[sceneName] = scenePath;
    }

    Scene& ScenesManager::getActiveScene() {
        return m_activeScene;
    }

    void ScenesManager::createDefaultScene() {
        //m_activeScene.clear();
        ResourcesManager& resourcesManager = m_systemsRegistry.getSystem<ResourcesManager>();
        Entity mainCamera = m_activeScene.createEntity("Main Camera");
        m_activeScene.addComponent(mainCamera, Camera());
        Entity cube = m_activeScene.createEntity("Cube");
        m_activeScene.addComponent(cube, MeshRenderer(resourcesManager.getDefaultMesh(), resourcesManager.getDefaultMaterial()));
        MeshRenderer& meshRenderer = m_activeScene.getComponent<MeshRenderer>(cube);
        meshRenderer.paintMesh();
    }
}
