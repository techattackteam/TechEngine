#include "Core.hpp"


#include "Timer.hpp"
#include "eventSystem/EventManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/CameraSystem.hpp"
#include "scene/SceneManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void Core::registerSystems(const std::filesystem::path& rootPath) {
        //m_systemRegistry.registerSystem<Project>(rootPath, m_systemRegistry);
        m_systemRegistry.registerSystem<EventManager>();
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<CameraSystem>(m_systemRegistry);
        m_systemRegistry.registerSystem<ResourceSystem>(m_systemRegistry);
        m_systemRegistry.registerSystem<SceneManager>(m_systemRegistry);
        m_systemRegistry.registerSystem<PhysicsEngine>(m_systemRegistry);
        m_systemRegistry.registerSystem<ScriptEngine>(m_systemRegistry);
    }

    void Core::init() {
        //m_systemRegistry.getSystem<Project>().init();
        m_systemRegistry.getSystem<EventManager>().init();
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<CameraSystem>().init();
        m_systemRegistry.getSystem<ResourceSystem>().init();
        m_systemRegistry.getSystem<SceneManager>().init();
        m_systemRegistry.getSystem<PhysicsEngine>().init();
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
        m_systemRegistry.onShutdown();
    }
}
