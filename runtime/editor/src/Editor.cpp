#include "Editor.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"

#include "resources/loaders/MeshLoader.hpp"
#include "eventSystem/EventManager.hpp"
#include "events/application/AppCloseEvent.hpp"
#include "panels/PanelsManager.hpp"
#include "simulator/RuntimeSimulator.hpp"
#include "window/Window.hpp"
#include "profiling/Profiler.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include "resources/loaders/MaterialLoader.hpp"
#include "resources/loaders/ModelLoader.hpp"
#include "resources/loaders/SceneLoader.hpp"
#include "resources/loaders/ShadersLoader.hpp"


namespace TechEngine {
    Editor::Editor() : Application(), m_entry(m_systemRegistry), m_textureLoader(nullptr) {
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
        m_systemRegistry.registerSystem<RuntimeSimulator<Client>>(m_client, m_systemRegistry);
        m_systemRegistry.registerSystem<RuntimeSimulator<Server>>(m_server, m_systemRegistry);

        m_systemRegistry.registerSystem<EventManager>();
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<ProjectManager>(m_systemRegistry, m_client.m_systemRegistry, m_server.m_systemRegistry);
        m_systemRegistry.registerSystem<Window>(m_systemRegistry);
        m_systemRegistry.registerSystem<FileSystem>();
        m_systemRegistry.registerSystem<Profiler>();

        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().registerSystems(m_lastProjectLoaded);
        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.registerSystem<Profiler>();
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().registerSystems(m_lastProjectLoaded);
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().m_runtime.m_systemRegistry.registerSystem<Profiler>();
        FileSystem& fileSystem = m_systemRegistry.getSystem<FileSystem>();
        m_fileWatcher = std::make_unique<FileWatcher>(fileSystem);
        m_textureLoader = new TextureLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.getSystem<ResourceSystem>(),
                                            m_systemRegistry.getSystem<FileSystem>());
        m_materialLoader = new MaterialLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.getSystem<ResourceSystem>(), m_systemRegistry.getSystem<FileSystem>());
        m_meshLoader = new MeshLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.getSystem<ResourceSystem>(), m_systemRegistry.getSystem<FileSystem>(), *m_textureLoader, *m_materialLoader);
        m_modelLoader = new ModelLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.getSystem<ResourceSystem>(), m_systemRegistry.getSystem<FileSystem>(), *m_textureLoader, *m_materialLoader, *m_meshLoader);
        m_sceneLoader = new SceneLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry, m_systemRegistry.getSystem<FileSystem>());
        m_shadersLoader = new ShadersLoader(m_systemRegistry.getSystem<RuntimeSimulator<Client>>().m_runtime.m_systemRegistry.getSystem<ResourceSystem>(), m_systemRegistry.getSystem<FileSystem>(), *m_fileWatcher);
        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().setSceneLoader(m_sceneLoader);
        m_systemRegistry.registerSystem<PanelsManager>(m_systemRegistry, m_client, m_server, m_textureLoader, m_materialLoader, m_modelLoader, m_meshLoader, m_sceneLoader, m_fileWatcher.get());
    }

    void Editor::init() {
        FileSystem& fileSystem = m_systemRegistry.getSystem<FileSystem>();
        m_systemRegistry.getSystem<ProjectManager>().init(m_lastProjectLoaded, {*m_textureLoader, *m_materialLoader, *m_meshLoader, *m_modelLoader, *m_sceneLoader, *m_shadersLoader}, fileSystem);
        m_systemRegistry.getSystem<Window>().init("TechEngineEditor - " + m_systemRegistry.getSystem<ProjectManager>().getProjectName(), 1280, 720);
        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<EventManager>().init();
        m_systemRegistry.getSystem<RuntimeSimulator<Client>>().init();
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().init();
        ComponentType<Tag>::get();
        ComponentType<Hierarchy>::get();
        ComponentType<Transform>::get();
        ComponentType<Camera>::get();
        ComponentType<PointLight>::get();
        ComponentType<DirectionalLight>::get();
        ComponentType<SpotLight>::get();
        ComponentType<MeshRenderer>::get();

        m_systemRegistry.getSystem<PanelsManager>().init();

        m_runFunction = [this]() {
            m_entry.run([this]() {
                            fixedUpdate();
                        }, [this]() {
                            update();
                        });
        };


        m_fileWatcher->watch("editorAssetsClient://", true);
        m_fileWatcher->subscribe([&](const FileChangedEvent& event) {
            //TE_LOGGER_INFO("File changed: {0}", event.virtualPath.string());
            if (event.action == FileChangeAction::Removed ||
                event.action == FileChangeAction::Renamed) {
                m_systemRegistry.getSystem<PanelsManager>().getContentBrowserPanel().validateCurrentPath();
            }
        });


        m_systemRegistry.getSystem<EventManager>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
            m_running = false;
            m_systemRegistry.getSystem<ProjectManager>().saveProject();
        });

        TE_LOGGER_INFO("Editor initialized");
        m_running = true;
    }

    void Editor::start() {
        m_systemRegistry.onStart();
    }

    void Editor::update() {
        if (m_fileWatcher) {
            m_fileWatcher->poll();
        }
        m_systemRegistry.getSystem<PanelsManager>().beginFrame();
        float deltaTime = m_systemRegistry.getSystem<Timer>().getDeltaTime();

        m_systemRegistry.onUpdate();

        auto& client = m_systemRegistry.getSystem<RuntimeSimulator<Client>>();
        auto& server = m_systemRegistry.getSystem<RuntimeSimulator<Server>>();
        client.tick(deltaTime);
        //server.tick(deltaTime);

        m_systemRegistry.getSystem<PanelsManager>().endFrame();
    }

    void Editor::fixedUpdate() {
        //m_systemRegistry.onFixedUpdate();
    }

    void Editor::stop() {
        //m_systemRegistry.onStop();
    }

    void Editor::shutdown() {
        m_systemRegistry.onShutdown();
    }

    Application* createApp() {
        return new Editor();
    }
}
