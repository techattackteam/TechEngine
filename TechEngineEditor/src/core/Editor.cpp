#include "Editor.hpp"
#include "core/FileSystem.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "script/ScriptRegister.hpp"

#include "yaml-cpp/yaml.h"
#include <fstream>

#include "events/input/KeyHoldEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"

namespace TechEngine {
    Editor::Editor() : AppCore(),
                       window(systemsRegistry, "TechEngineEditor", 1600, 900),
                       client(window),
                       server(),
                       panelsManager(client, server, systemsRegistry) {
        systemsRegistry.registerSystem<ProjectManager>(client, server);
        systemsRegistry.getSystem<Logger>().init("TechEngineEditor");
        editorSettings = FileSystem::rootPath.string() + "/EditorSettings.TESettings";
        systemsRegistry.getSystem<ProjectManager>().init();
        loadEditorSettings();
        client.init();
        server.init();
        systemsRegistry.getSystem<ProjectManager>().setupPaths();
        client.systemsRegistry.getSystem<Renderer>().init(client.project.getResourcesPath().string());
        server.systemsRegistry.getSystem<Renderer>().init(server.project.getResourcesPath().string());
        panelsManager.init();
        client.systemsRegistry.getSystem<SceneManager>().loadScene(client.project.lastLoadedScene);
        server.systemsRegistry.getSystem<SceneManager>().loadScene(server.project.lastLoadedScene);
        systemsRegistry.getSystem<EventDispatcher>().subscribe(KeyPressedEvent::eventType, [this](Event* event) {
            Key key = ((KeyPressedEvent*)event)->getKey();
            client.systemsRegistry.getSystem<EventDispatcher>().dispatch(new KeyPressedEvent(key));
        });

        systemsRegistry.getSystem<EventDispatcher>().subscribe(KeyReleasedEvent::eventType, [this](Event* event) {
            Key key = ((KeyReleasedEvent*)event)->getKey();
            client.systemsRegistry.getSystem<EventDispatcher>().dispatch(new KeyReleasedEvent(key));
        });

        systemsRegistry.getSystem<EventDispatcher>().subscribe(KeyHoldEvent::eventType, [this](Event* event) {
            Key key = ((KeyHoldEvent*)event)->getKey();
            client.systemsRegistry.getSystem<EventDispatcher>().dispatch(new KeyHoldEvent(key));
        });

        systemsRegistry.getSystem<EventDispatcher>().subscribe(MouseScrollEvent::eventType, [this](Event* event) {
            float xOffset = ((MouseScrollEvent*)event)->getXOffset();
            float yOffset = ((MouseScrollEvent*)event)->getYOffset();
            client.systemsRegistry.getSystem<EventDispatcher>().dispatch(new MouseScrollEvent(xOffset, yOffset));
        });

        systemsRegistry.getSystem<EventDispatcher>().subscribe(MouseMoveEvent::eventType, [this](Event* event) {
            glm::vec2 fromPos = ((MouseMoveEvent*)event)->getFromPosition();
            glm::vec2 toPos = ((MouseMoveEvent*)event)->getToPosition();
            client.systemsRegistry.getSystem<EventDispatcher>().dispatch(new MouseMoveEvent(fromPos, toPos));
        });
    }

    void Editor::onUpdate() {
        systemsRegistry.getSystem<EventDispatcher>().syncEventManager.execute();
        client.onUpdate();
        server.onUpdate();
        panelsManager.update();
    }

    void Editor::onFixedUpdate() {
        systemsRegistry.getSystem<EventDispatcher>().fixedSyncEventManager.execute();
        client.onFixedUpdate();
        server.onFixedUpdate();
        systemsRegistry.getSystem<PhysicsEngine>().onFixedUpdate();
    }

    void Editor::loadEditorSettings() {
        std::string lastProjectLoaded;
        if (std::filesystem::exists(editorSettings)) {
            YAML::Node data;
            try {
                data = YAML::LoadFile(editorSettings);
                lastProjectLoaded = data["Last project loaded"].as<std::string>();
                if (std::filesystem::exists(lastProjectLoaded)) {
                    systemsRegistry.getSystem<ProjectManager>().loadEditorProject(lastProjectLoaded);
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
        systemsRegistry.getSystem<ProjectManager>().createNewProject("DefaultProject");
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Last project loaded" << YAML::Value << FileSystem::rootPath.string() + "\\DefaultProject";
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::ofstream fout(editorSettings);
        fout << out.c_str();
        systemsRegistry.getSystem<ProjectManager>().loadEditorProject(FileSystem::rootPath.string() + "\\DefaultProject");
    }
}
