#include <fstream>
#include "Editor.hpp"
#include "core/FileSystem.hpp"
#include "external/EntryPoint.hpp"
#include "yaml-cpp/yaml.h"

namespace TechEngine {
    Editor::Editor() : AppCore(),
                       window(eventDispatcher, "TechEngineEditor", 1600, 900),
                       client(window),
                       server(),
                       projectManager(client, server),
                       panelsManager(client, server, eventDispatcher, projectManager) {
        editorSettings = FileSystem::rootPath.string() + "/EditorSettings.TESettings";
        projectManager.init();
        loadEditorSettings();
        panelsManager.init();
        client.renderer.init(client.filePaths);
        server.init();
    }

    void Editor::onUpdate() {
        eventDispatcher.syncEventManager.execute();
        client.onUpdate();
        server.onUpdate();
        panelsManager.update();
    }

    void Editor::onFixedUpdate() {
        eventDispatcher.fixedSyncEventManager.execute();
        client.onFixedUpdate();
        server.onFixedUpdate();
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
                } else {
                    TE_LOGGER_INFO("Unable to load {0}.\n Creating new project.", lastProjectLoaded);
                    createNewProject();
                }
            } catch (YAML::Exception& e) {
                TE_LOGGER_CRITICAL("Failed to load EditorSettings.TESettings file.\n      {0}", e.what());
            }
            return;
        }
        createNewProject();
    }

    void Editor::createNewProject() {
        projectManager.createNewProject("DefaultProject");
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Last project loaded" << YAML::Value << FileSystem::rootPath.string() + "\\DefaultProject";
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::ofstream fout(editorSettings);
        fout << out.c_str();
        projectManager.loadEditorProject(FileSystem::rootPath.string() + "\\DefaultProject");
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::Editor();
}
