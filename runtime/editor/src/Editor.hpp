#pragma once
#include <Application.hpp>
#include <core/Client.hpp>
#include <core/Server.hpp>
#include "fileSystem/FileWatcher.hpp"
#include "resources/loaders/TextureLoader.hpp"

#include <filesystem>
#include <memory>


namespace TechEngine {
    class ShadersLoader;

    class Editor : public Application {
    private:
        Entry m_entry;
        Server m_server;
        Client m_client;
        SystemsRegistry m_systemRegistry;

        TextureLoader* m_textureLoader;
        MaterialLoader* m_materialLoader;
        MeshLoader* m_meshLoader;
        ModelLoader* m_modelLoader;
        SceneLoader* m_sceneLoader;
        ShadersLoader* m_shadersLoader;
        std::unique_ptr<FileWatcher> m_fileWatcher;

        std::filesystem::path m_EditorConfigPath;
        std::filesystem::path m_lastProjectLoaded;

    public:
        Editor();

        void loadEditorConfig();

        void registerSystems() override;

        void init() override;

        void start() override;

        void update() override;

        void fixedUpdate() override;

        void stop() override;

        void shutdown() override;
    };
}
