#include <fstream>
#include "Editor.hpp"
#include "core/FileSystem.hpp"
#include "external/EntryPoint.hpp"
#include "script/ScriptEngine.hpp"
#include "yaml-cpp/yaml.h"

namespace TechEngine {
    Editor::Editor() : Client("TechEngine", RendererSettings::width, RendererSettings::height), panelsManager(window, sceneManager, projectManager, physicsEngine, textureManager, materialManager, renderer) {
        ScriptEngine* scriptEngine = new ScriptEngine(false);
        editorSettings = FileSystem::rootPath.string() + "/EditorSettings.TESettings";
        projectManager.init();
        loadEditorSettings();
        panelsManager.init();
        renderer.init(projectManager);
    }

    void Editor::onUpdate() {
        panelsManager.update();
        window.onUpdate();
    }

    void Editor::onFixedUpdate() {
        physicsEngine.onFixedUpdate();
    }

    void Editor::loadEditorSettings() {
        std::string lastProjectLoaded;
        if (std::filesystem::exists(editorSettings)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(editorSettings);
                lastProjectLoaded = data["Last project loaded"].as<std::string>();
                if (std::filesystem::exists(lastProjectLoaded)) {
                    projectManager.loadEditorProject(lastProjectLoaded);
                }
            } catch (YAML::Exception& e) {
                TE_LOGGER_CRITICAL("Failed to load EditorSettings.TESettings file.\n      {0}", e.what());
            }
            return;
        }
        projectManager.createNewProject("New Project");
        lastProjectLoaded = FileSystem::rootPath.string() + "\\New Project";
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Last project loaded" << YAML::Value << lastProjectLoaded;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::ofstream fout(editorSettings);
        fout << out.c_str();
        projectManager.loadEditorProject(lastProjectLoaded);
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::Editor();
}
