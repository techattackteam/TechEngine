#include <yaml-cpp/yaml.h>
#include <yaml-cpp/exceptions.h>
#include "TechEngineRuntime.hpp"
#include "renderer/RendererSettings.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"
#include "core/Logger.hpp"
#include "core/FileSystem.hpp"

namespace TechEngine {
    TechEngineRuntime::TechEngineRuntime() : App("TechEngine", RendererSettings::width, RendererSettings::height) {
        EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngine::Event* event) {
            onWindowResizeEvent((WindowResizeEvent*)event);
        });
        if (!loadRendererSettings()) {
            EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            return;
        }
        renderer.init(projectManager);
        ScriptEngine* scriptEngine = new ScriptEngine();
#ifdef TE_DEBUG
        ScriptEngine::getInstance()->init(projectManager.getScriptsDebugDLLPath().string());
#elif TE_RELEASE
        ScriptEngine::getInstance()->init(projectManager.getScriptsReleaseDLLPath().string());
#endif
        ScriptEngine::getInstance()->onStart();
    }

    void TechEngineRuntime::onUpdate() {
        ScriptEngine::getInstance()->onUpdate();
        renderer.renderPipeline(sceneManager.getScene());
        window.onUpdate();
    }

    void TechEngineRuntime::onFixedUpdate() {
        ScriptEngine::getInstance()->onFixedUpdate();
        physicsEngine.onFixedUpdate();
    }

    bool TechEngineRuntime::loadRendererSettings() {
        projectManager.loadRuntimeProject(std::filesystem::current_path().string());
        window.changeTitle(projectManager.getProjectName());
        RendererSettings::resize(1080, 720);
        return true;
    }


    void TechEngineRuntime::onWindowResizeEvent(WindowResizeEvent* event) {
        RendererSettings::resize(event->getWidth(), event->getHeight());
        glViewport(0, 0, event->getWidth(), event->getHeight());
    }
}


TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::TechEngineRuntime();
}
