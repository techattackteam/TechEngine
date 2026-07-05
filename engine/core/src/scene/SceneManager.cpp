#include "SceneManager.hpp"

#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    SceneManager::SceneManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), m_activeScene(systemsRegistry) {
    }

    void SceneManager::init() {
    }

    void SceneManager::shutdown() {
    }

    Scene& SceneManager::getActiveScene() {
        return m_activeScene;
    }

    const std::string& SceneManager::getActiveSceneName() const {
        return m_activeScene.getName();
    }

    void SceneManager::clear() {
        m_activeScene.clear();
    }
}
