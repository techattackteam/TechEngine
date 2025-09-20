#include "Editor.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>


#include "imgui.h"
#include "eventSystem/EventDispatcher.hpp"
#include "events/application/AppCloseEvent.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshCreatedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshDeletedEvent.hpp"
#include "input/Input.hpp"
#include "logger/ImGuiSink.hpp"
#include "panels/PanelsManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "simulator/RuntimeSimulator.hpp"
#include "window/Window.hpp"

namespace TechEngine {
    Editor::Editor() : Application(), m_entry(m_systemRegistry) {
    }

    void Editor::loadEditorConfig() {
        if (!std::filesystem::exists(std::filesystem::current_path().string() + "\\EditorConfig.tecfg")) {
            TE_LOGGER_ERROR("Editor config not found");
            std::ofstream fout(std::filesystem::current_path().string() + "\\EditorConfig.tecfg");
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Last Project Loaded" << YAML::Value << std::filesystem::current_path().string() + "\\New Project";
            out << YAML::EndSeq;
            out << YAML::EndMap;
            fout << out.c_str();
            TE_LOGGER_INFO("New editor config created");
        }
        YAML::Node config = YAML::LoadFile(std::filesystem::current_path().string() + "\\EditorConfig.tecfg");
        if (!config["Last Project Loaded"].IsDefined() || !std::filesystem::exists(config["Last Project Loaded"].as<std::string>())) {
            TE_LOGGER_ERROR("Last Project Loaded not found in editor config!\n Creating new default project");
        }
        m_lastProjectLoaded = config["Last Project Loaded"].as<std::string>();
    }

    void Editor::registerSystems() {
        m_systemRegistry.registerSystem<Logger>("TechEngineEditor");
        m_systemRegistry.getSystem<Logger>().init();
        loadEditorConfig();
        m_systemRegistry.registerSystem<EventDispatcher>();
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<ProjectManager>(m_client.m_systemRegistry, m_server.m_systemRegistry);
        m_systemRegistry.registerSystem<Window>(m_systemRegistry);
        m_systemRegistry.registerSystem<PanelsManager>(m_systemRegistry, m_client, m_server);
        m_systemRegistry.registerSystem<RuntimeSimulator<Client>>(m_client, m_systemRegistry);
        m_systemRegistry.registerSystem<RuntimeSimulator<Server>>(m_server, m_systemRegistry);
        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().registerSystems(m_lastProjectLoaded);
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().registerSystems(m_lastProjectLoaded);
    }

    void Editor::init() {
        m_systemRegistry.getSystem<ProjectManager>().init(m_lastProjectLoaded);
        m_systemRegistry.getSystem<Window>().init("TechEngineEditor - " + m_systemRegistry.getSystem<ProjectManager>().getProjectName(), 1280, 720);
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<EventDispatcher>().init();

        m_client.m_systemRegistry.getSystem<EventDispatcher>().registerEditorWatchDog([this](const std::shared_ptr<Event>& event) {
            EventDispatcher& manager = m_systemRegistry.getSystem<EventDispatcher>();
            if (std::dynamic_pointer_cast<MeshCreatedEvent>(event) ||
                std::dynamic_pointer_cast<MeshDeletedEvent>(event) ||
                std::dynamic_pointer_cast<MaterialCreatedEvent>(event) ||
                std::dynamic_pointer_cast<MaterialDeletedEvent>(event)
            ) {
                manager.executeSingleEvent(event);
            }
        });
        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().init();

        m_server.m_systemRegistry.getSystem<EventDispatcher>().registerEditorWatchDog([this](const std::shared_ptr<Event>& event) {
            EventDispatcher& manager = m_systemRegistry.getSystem<EventDispatcher>();
        });
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().init();
        ComponentType::init(); // Initialize component types for the editor
        m_systemRegistry.getSystem<PanelsManager>().init();


        m_runFunction = [this]() {
            m_entry.run([this]() {
                            fixedUpdate();
                        }, [this]() {
                            update();
                        });
        };

        TE_LOGGER_INFO("Editor initialized");
        m_systemRegistry.getSystem<EventDispatcher>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
            m_running = false;
            m_systemRegistry.getSystem<ProjectManager>().saveProject();
        });

        m_running = true;
    }

    void Editor::start() {
        m_systemRegistry.onStart();
    }

    void Editor::update() {
        m_systemRegistry.onUpdate();
    }

    void Editor::fixedUpdate() {
        m_systemRegistry.onFixedUpdate();
    }

    void Editor::stop() {
        m_systemRegistry.onStop();
    }

    void Editor::shutdown() {
        m_systemRegistry.onShutdown();
    }

    Application* createApp() {
        return new Editor();
    }
}
