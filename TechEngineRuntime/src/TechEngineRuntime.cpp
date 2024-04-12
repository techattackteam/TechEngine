#include "TechEngineRuntime.hpp"
#include "external/EntryPoint.hpp"
#include "core/Client.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    TechEngineRuntime::TechEngineRuntime() : Client("TechEngine", 1080, 720) {
        EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](Event* event) {
            onWindowResizeEvent((WindowResizeEvent*)event);
        });

        if (!loadRendererSettings()) {
            EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            return;
        }
        renderer.init(projectManager);
        ScriptEngine* scriptEngine = new ScriptEngine(true);
#ifdef TE_DEBUG
        ScriptEngine::getInstance()->init(projectManager.getScriptsDebugDLLPath().string());
#elif TE_RELEASEDEBUG
        ScriptEngine::getInstance()->init(projectManager.getScriptsReleaseDLLPath().string());
#elif TE_RELEASE
        ScriptEngine::getInstance()->init(projectManager.getScriptsReleaseDLLPath().string());
#endif
        ScriptEngine::getInstance()->onStart();
        physicsEngine.start();
    }

    void TechEngineRuntime::onUpdate() {
        ScriptEngine::getInstance()->onUpdate();
        renderer.renderPipeline(sceneManager.getScene().getMainCamera());
        window.onUpdate();
    }

    void TechEngineRuntime::onFixedUpdate() {
        ScriptEngine::getInstance()->onFixedUpdate();
        physicsEngine.onFixedUpdate();
    }

    bool TechEngineRuntime::loadRendererSettings() {
        projectManager.loadRuntimeProject(std::filesystem::current_path().string());
        window.changeTitle(projectManager.getProjectName());
        float width = 1080;
        float height = 720;
        float aspectRatio = width / height;
        glViewport(0, 0, width, height);
        sceneManager.getScene().getMainCamera()->updateProjectionMatrix(aspectRatio);
        return true;
    }


    void TechEngineRuntime::onWindowResizeEvent(WindowResizeEvent* event) {
        float aspectRatio = (float)event->getWidth() / (float)event->getHeight();
        glViewport(0, 0, event->getWidth(), event->getHeight());
        sceneManager.getScene().getMainCamera()->updateProjectionMatrix(aspectRatio);
    }
}


TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::TechEngineRuntime();
}
