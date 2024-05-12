#include "Editor.hpp"
#include "core/FileSystem.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "external/EntryPoint.hpp"
#include "script/ScriptRegister.hpp"

#include "yaml-cpp/yaml.h"
#include <fstream>

#include "events/input/KeyHoldEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"

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
        ScriptRegister::getInstance().init(&client.scriptEngine, &server.scriptEngine);

        eventDispatcher.subscribe(KeyPressedEvent::eventType, [this](Event* event) {
            Key key = ((KeyPressedEvent*)event)->getKey();
            client.eventDispatcher.dispatch(new KeyPressedEvent(key));
        });

        eventDispatcher.subscribe(KeyReleasedEvent::eventType, [this](Event* event) {
            Key key = ((KeyReleasedEvent*)event)->getKey();
            client.eventDispatcher.dispatch(new KeyReleasedEvent(key));
        });

        eventDispatcher.subscribe(KeyHoldEvent::eventType, [this](Event* event) {
            Key key = ((KeyHoldEvent*)event)->getKey();
            client.eventDispatcher.dispatch(new KeyHoldEvent(key));
        });

        eventDispatcher.subscribe(MouseScrollEvent::eventType, [this](Event* event) {
            float xOffset = ((MouseScrollEvent*)event)->getXOffset();
            float yOffset = ((MouseScrollEvent*)event)->getYOffset();
            client.eventDispatcher.dispatch(new MouseScrollEvent(xOffset, yOffset));
        });

        eventDispatcher.subscribe(MouseMoveEvent::eventType, [this](Event* event) {
            glm::vec2 fromPos = ((MouseMoveEvent*)event)->getFromPosition();
            glm::vec2 toPos = ((MouseMoveEvent*)event)->getToPosition();
            client.eventDispatcher.dispatch(new MouseMoveEvent(fromPos, toPos));
        });
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
