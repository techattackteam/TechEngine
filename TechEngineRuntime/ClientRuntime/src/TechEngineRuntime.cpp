#include "TechEngineRuntime.hpp"

#include "core/Logger.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "physics/PhysicsEngine.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    TechEngineRuntime::TechEngineRuntime() : window(systemsRegistry, "TechEngine", 1080, 720), Client(window) {
        systemsRegistry.registerSystem<Renderer>();
    }

    void TechEngineRuntime::init() {
        project.loadProject(std::filesystem::current_path().string(), ProjectType::Client);
        Client::init();
        systemsRegistry.getSystem<Renderer>().init(project.getResourcesPath().string());
        systemsRegistry.getSystem<SceneManager>().loadScene(project.lastLoadedScene);
        systemsRegistry.getSystem<EventDispatcher>().subscribe(WindowResizeEvent::eventType, [this](Event* event) {
            onWindowResizeEvent((WindowResizeEvent*)event);
        });

        if (!loadRendererSettings()) {
            systemsRegistry.getSystem<EventDispatcher>().dispatch(new AppCloseRequestEvent());
            return;
        }
#ifdef TE_DEBUG
        systemsRegistry.getSystem<ScriptEngine>().init(project.getUserScriptsDLLPath().string(), &systemsRegistry.getSystem<EventDispatcher>());
        TE_LOGGER_INFO("Debug");
#elif TE_RELEASE
        systemsRegistry.getSystem<ScriptEngine>().init(project.getUserScriptsDLLPath().string(), &systemsRegistry.getSystem<EventDispatcher>());
        TE_LOGGER_INFO("Release");
#endif
        systemsRegistry.getSystem<ScriptEngine>().onStart();
        systemsRegistry.getSystem<PhysicsEngine>().start();
    }

    void TechEngineRuntime::onUpdate() {
        Client::onUpdate();
        systemsRegistry.getSystem<Renderer>().renderPipeline(systemsRegistry.getSystem<SceneManager>().getScene(), systemsRegistry.getSystem<SceneManager>().getScene().getMainCamera());
    }

    void TechEngineRuntime::onFixedUpdate() {
        Client::onFixedUpdate();
        systemsRegistry.getSystem<PhysicsEngine>().onFixedUpdate();
    }

    bool TechEngineRuntime::loadRendererSettings() {
        window.changeTitle(project.getProjectName());
        float width = 1080;
        float height = 720;
        float aspectRatio = width / height;
        glViewport(0, 0, width, height);
        systemsRegistry.getSystem<SceneManager>().getScene().getMainCamera()->updateProjectionMatrix(aspectRatio);
        return true;
    }


    void TechEngineRuntime::onWindowResizeEvent(WindowResizeEvent* event) {
        float aspectRatio = (float)event->getWidth() / (float)event->getHeight();
        glViewport(0, 0, event->getWidth(), event->getHeight());
        systemsRegistry.getSystem<SceneManager>().getScene().getMainCamera()->updateProjectionMatrix(aspectRatio);
    }
}
