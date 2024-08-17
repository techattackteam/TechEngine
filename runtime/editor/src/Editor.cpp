#include "Editor.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "project/ProjectManager.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>

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

    void Editor::init() {
        m_systemRegistry.registerSystem<Logger>("TechEngineEditor");
        m_systemRegistry.getSystem<Logger>().init();

        m_systemRegistry.registerSystem<Timer>();
        loadEditorConfig();
        m_systemRegistry.registerSystem<ProjectManager>(m_lastProjectLoaded);

        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<ProjectManager>().init();
        m_runFunction = [this]() {
            m_entry.run([this]() {
                            fixedUpdate();
                        }, [this]() {
                            update();
                        });
        };
        m_client.init();
        m_server.init();
        m_running = true;
    }

    void Editor::start() {
        m_systemRegistry.getSystem<Timer>().onStart();
        m_client.onStart();
        m_server.onStart();
        TE_LOGGER_INFO("Editor started");
    }

    void Editor::update() {
    }

    void Editor::fixedUpdate() {
    }

    void Editor::stop() {
    }

    void Editor::destroy() {
        m_client.destroy();
        m_server.destroy();
    }

    Application* createApp() {
        return new Editor();
    }
}
