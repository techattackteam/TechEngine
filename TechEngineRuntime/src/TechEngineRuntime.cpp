#include <yaml-cpp/yaml.h>
#include <iostream>
#include <filesystem>
#include <yaml-cpp/exceptions.h>
#include "TechEngineRuntime.hpp"
#include "renderer/RendererSettings.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {

    TechEngineRuntime::TechEngineRuntime() : App() {
        if (!loadRendererSettings()) {
            TechEngine::EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            return;
        }
        window.init(windowName, width, height);
        //TODO: FIX THIS
        //SceneManager::deserialize(std::filesystem::current_path().string() + "/scenes/" + sceneToLoadName + ".scene");
        TechEngine::EventDispatcher::getInstance().syncEventManager.execute();
        SceneHelper::mainCamera = Scene::getInstance().getGameObject("SceneCamera")->getComponent<CameraComponent>();
        window.getRenderer().init();
        ScriptEngine::getInstance()->init(std::filesystem::current_path().string() + "/UserProject.dll");
    }

    void TechEngineRuntime::onUpdate() {
        Mouse::getInstance().onUpdate();
        window.getRenderer().renderPipeline();
        window.onUpdate();
    }

    void TechEngineRuntime::onFixedUpdate() {

    }

    bool TechEngineRuntime::loadRendererSettings() {
        YAML::Node data;
        try {
            data = YAML::LoadFile(std::filesystem::current_path().string() + "/ExportSettings.TESettings");
        } catch (YAML::Exception &e) {
            std::cout << "Failed to load .TESettings file " << "\n     " << e.what() << std::endl;
            return false;
        }
        auto rendererSettingsNode = data["Window settings"];
        windowName = rendererSettingsNode["name"].as<std::string>();
        width = rendererSettingsNode["width"].as<uint32_t>();
        height = rendererSettingsNode["height"].as<uint32_t>();

        RendererSettings::resize(width, height);

        sceneToLoadName = data["Default Scene"].as<std::string>();
        return true;
    }
}

TechEngine::AppCore *TechEngine::createApp() {
    return new TechEngine::TechEngineRuntime();
}