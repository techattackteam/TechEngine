#pragma once
#include <Application.hpp>
#include <core/Client.hpp>
#include <core/Server.hpp>
#include <filesystem>
#include <gl/glew.h>
#include <glfw/glfw3.h>

namespace TechEngine {
    class Editor : public Application {
    private:
        Entry m_entry;
        Server m_server;
        Client m_client;
        SystemsRegistry m_systemRegistry;
        std::filesystem::path m_EditorConfigPath;
        std::filesystem::path m_lastProjectLoaded;
        GLFWwindow* handler;

    public:
        Editor();

        void loadEditorConfig();

        void init() override;

        void start() override;

        void update() override;

        void fixedUpdate() override;

        void stop() override;

        void destroy() override;
    };
}
