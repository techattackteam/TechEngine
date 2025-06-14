#include "RuntimeClient.hpp"

#include "core/Logger.hpp"
#include "events/application/AppCloseEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "project/Project.hpp"
#include "renderer/Renderer.hpp"
#include "scene/ScenesManager.hpp"
#include "script/ScriptEngine.hpp"
#include "window/Window.hpp"

namespace TechEngine {
    void RuntimeClient::registerSystems() {
        m_client.registerSystems(std::filesystem::current_path());
        m_client.m_systemRegistry.registerSystem<Window>(m_client.m_systemRegistry);
    }

    void RuntimeClient::init() {
        m_client.m_systemRegistry.getSystem<Window>().init("TechEngineRuntime", 1280, 720);
        std::filesystem::path path = std::filesystem::current_path();
        m_client.m_systemRegistry.getSystem<Project>().loadRuntimeProject(path);
        m_client.init();
        m_runFunction = [this]() {
            m_client.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_client.m_systemRegistry.getSystem<ScriptEngine>().start(
            m_client.m_systemRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string() + "\\client\\scripts\\build\\debug\\ClientScripts.dll");

        m_client.m_systemRegistry.getSystem<EventDispatcher>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
            m_running = false;
            m_client.m_systemRegistry.getSystem<ScenesManager>().saveScene();
        });
    }

    void RuntimeClient::start() {
        m_running = true;
        m_client.onStart();
    }

    void RuntimeClient::fixedUpdate() {
        m_client.onFixedUpdate();
    }

    void RuntimeClient::update() {
        m_client.onUpdate();
        Scene& scene = m_client.m_systemRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, Camera>([this](Transform& transform, Camera& camera) {
            Renderer& renderer = m_client.m_systemRegistry.getSystem<Renderer>();
            camera.updateProjectionMatrix(1280.0f / 720.0f);
            camera.updateViewMatrix(transform.getModelMatrix());
            renderer.renderPipeline(camera);
        });
    }

    void RuntimeClient::stop() {
        m_client.onStop();
    }

    void RuntimeClient::shutdown() {
        m_client.shutdown();
    }

    Application* createApp() {
        return new RuntimeClient();
    }
}
