#include <fstream>
#include "Editor.hpp"
#include "core/FileSystem.hpp"
#include "yaml-cpp/yaml.h"

namespace TechEngine {

    Editor::Editor() : App("TechEngine", RendererSettings::width, RendererSettings::height), panelsManager(window, sceneManager, projectManager, physicsEngine, textureManager, materialManager) {

        editorSettings = FileSystem::rootPath.string() + "/EditorSettings.TESettings";
        projectManager.init();
        loadEditorSettings();
        panelsManager.init();
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
                    projectManager.loadProject(lastProjectLoaded);
                    return;
                }
            }
            catch (YAML::Exception &e) {
                TE_LOGGER_CRITICAL("Failed to load EditorSettings.TESettings file.\n      {0}", e.what());
            }
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
        projectManager.loadProject(lastProjectLoaded);
    }

}

TechEngine::AppCore *TechEngine::createApp() {
    return new TechEngine::Editor();
}

