#include <yaml-cpp/yaml.h>
#include <yaml-cpp/exceptions.h>
#include "TechEngineRuntime.hpp"
#include "renderer/RendererSettings.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"
#include "core/Logger.hpp"
#include "core/FileSystem.hpp"

namespace TechEngine {

    TechEngineRuntime::TechEngineRuntime() : App("TechEngine", RendererSettings::width, RendererSettings::height) {
        EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngine::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) event);
        });
        if (!loadRendererSettings()) {
            EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            return;
        }
        ScriptEngine::getInstance()->init(projectManager.getProjectLocation().string() + "/UserProject.dll");
    }

    void TechEngineRuntime::onUpdate() {
        window.getRenderer().renderPipeline(sceneManager.getScene());
        window.onUpdate();
    }

    void TechEngineRuntime::onFixedUpdate() {
        physicsEngine.onFixedUpdate();
    }

    bool TechEngineRuntime::loadRendererSettings() {
        projectManager.loadProject(FileSystem::getAllFilesWithExtension(std::filesystem::current_path().string(), ".teprj")[0]);
        YAML::Node data;
        try {
            data = YAML::LoadFile(projectManager.getProjectLocation().string() + "/Export.texp");
        } catch (YAML::Exception &e) {
            TE_LOGGER_ERROR("Failed to load .TESettings file \n {0}", e.what());
            return false;
        }

        auto rendererSettingsNode = data["Window settings"];

        windowName = rendererSettingsNode["name"].as<std::string>();
        width = rendererSettingsNode["width"].as<uint32_t>();
        height = rendererSettingsNode["height"].as<uint32_t>();

        window.changeTitle(windowName);
        RendererSettings::resize(width, height);
        sceneToLoadName = data["Default Scene"].as<std::string>();
        return true;
    }


    void TechEngineRuntime::onWindowResizeEvent(WindowResizeEvent *event) {
        RendererSettings::resize(event->getWidth(), event->getHeight());
        glViewport(0, 0, event->getWidth(), event->getHeight());
    }
}


TechEngine::AppCore *TechEngine::createApp() {
    return new TechEngine::TechEngineRuntime();
}