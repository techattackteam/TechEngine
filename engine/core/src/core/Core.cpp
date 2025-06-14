#include "Core.hpp"


#include "Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"
//#include "physics/PhysicsEngine.hpp"
#include "physics/PhysicsEngine.hpp"
#include "project/Project.hpp"
#include "resources/ResourcesManager.hpp"
#include "scene/CameraSystem.hpp"
#include "scene/ScenesManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void Core::registerSystems(const std::filesystem::path& rootPath) {
        m_systemRegistry.registerSystem<Project>(rootPath, m_systemRegistry);
        m_systemRegistry.registerSystem<EventDispatcher>();
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<CameraSystem>(m_systemRegistry);
        m_systemRegistry.registerSystem<ResourcesManager>(m_systemRegistry);
        m_systemRegistry.registerSystem<PhysicsEngine>(m_systemRegistry);
        m_systemRegistry.registerSystem<ScenesManager>(m_systemRegistry);
        m_systemRegistry.registerSystem<ScriptEngine>(m_systemRegistry);
    }

    void Core::init(AppType appType) {
        m_systemRegistry.getSystem<Project>().init();
        m_systemRegistry.getSystem<EventDispatcher>().init();
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<CameraSystem>().init();
        m_systemRegistry.getSystem<ResourcesManager>().init(appType);
        m_systemRegistry.getSystem<PhysicsEngine>().init();
        m_systemRegistry.getSystem<ScenesManager>().init(appType);
        m_systemRegistry.getSystem<ScriptEngine>().init();
    }

    void Core::onStart() {
        m_systemRegistry.onStart();
    }

    void Core::onFixedUpdate() {
        m_systemRegistry.onFixedUpdate();
    }

    void Core::onUpdate() {
        m_systemRegistry.onUpdate();
    }

    void Core::onStop() {
        m_systemRegistry.onStop();
    }

    void Core::shutdown() {
        ScenesManager& scenesManager = m_systemRegistry.getSystem<ScenesManager>();
        scenesManager.saveScene();
        m_systemRegistry.onShutdown();
    }
}
