#include <fstream>
#include "Editor.hpp"
#include "project/ProjectManager.hpp"
#include "yaml-cpp/yaml.h"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {

    Editor::Editor() : panelsManager(window) {
        ProjectManager::init(std::filesystem::current_path());
        rootPath = ProjectManager::getRootPath();
        editorSettings = rootPath.string() + "/EditorSettings.TESettings";
        PhysicsEngine::getInstance()->init();
        loadEditorSettings();
        panelsManager.init();
    }

    void Editor::onUpdate() {
        PhysicsEngine::getInstance()->onUpdate();
        panelsManager.update();
        window.onUpdate();
    }

    void Editor::onFixedUpdate() {

    }

    void Editor::loadEditorSettings() {
        std::string lastProjectLoaded;
        if (std::filesystem::exists(editorSettings)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(editorSettings.string());
                lastProjectLoaded = data["Last project loaded"].as<std::string>();
                if (std::filesystem::exists(lastProjectLoaded)) {
                    ProjectManager::loadProject(lastProjectLoaded);
                    return;
                }
            }
            catch (YAML::Exception &e) {
                TE_LOGGER_CRITICAL("Failed to load EditorSettings.TESettings file.\n      {0}", e.what());
            }
        }
        ProjectManager::createNewProject("New Project");
        lastProjectLoaded = rootPath.string() + "\\New Project";
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Last project loaded" << YAML::Value << lastProjectLoaded;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::ofstream fout(editorSettings.string());
        fout << out.c_str();
        ProjectManager::loadProject(lastProjectLoaded);
    }
}

TechEngine::AppCore *TechEngine::createApp() {
    return new TechEngine::Editor();
}

