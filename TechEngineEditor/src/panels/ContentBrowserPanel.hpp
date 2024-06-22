#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "assimp/AssimpLoader.hpp"
#include "core/Server.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class PanelsManager;

    enum FileType {
        CLIENT_FILE,
        SERVER_FILE,
        COMMON_FILE
    };

    class ContentBrowserPanel : public Panel {
    private:
        std::filesystem::path currentPath;
        std::filesystem::path selectedPath;

        Client& client;
        Server& server;
        SystemsRegistry& editorRegistry;
        PanelsManager& panelsManager;
        //AssimpLoader assimpLoader; //TODO: change the way we load models

    public:
        ContentBrowserPanel(Client& client, Server& server, SystemsRegistry& editorRegistry, PanelsManager& panelsManager);

        void init();

        void onUpdate() override;

        void openFolderOrFile(const std::string& filename);

        void renameFile(const std::string& filename, const std::string& newName);

        void tryLoadModel(const std::string& filepath);

        void deleteFile(const std::string& filename);

        void renderDirectoryHierarchy(const std::filesystem::path& directoryPath);

        void renderDirectoryHierarchyRecurse(const std::filesystem::path& path);

        void renderFiles(const std::filesystem::path& directoryPath);

        bool openMenuPopupItem(const std::filesystem::path& path);

        bool openMenuPopupWindow(const std::filesystem::path& path);

        void runFunctionBasedOnFileType(std::string path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction);

    private:
        FileType getFileType(const std::filesystem::path& path);
    };
}
